#include "ZstClient.h"
#include <boost/uuid/uuid_io.hpp>
#include <boost/coroutine2/coroutine.hpp>
#include <boost/dll.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/spawn.hpp>

using namespace flatbuffers;
using namespace std::chrono_literals;

namespace showtime::client
{

ZstClient::ZstClient(ShowtimeClient* api) :
	m_ping(-1),
    m_is_ending(false),
    m_is_destroyed(false),
    m_init_completed(false),
    m_connected_to_stage(false),
    m_is_connecting(false),
    m_service_broadcast_transport(std::make_shared<ZstServiceDiscoveryTransport>()),
    m_auto_join_stage(false),

    //Modules
    m_session(std::make_shared<ZstClientSession>()),
    m_plugins(std::make_shared<ZstPluginLoader>()),

    //Transports
    m_tcp_graph_transport(nullptr),
    m_udp_graph_transport(nullptr),
    m_stun_srv(std::make_shared<ZstSTUNService>()),
    m_client_transport(std::make_shared<ZstZMQClientTransport>()),

    //Timers
    m_heartbeat_timer(m_client_timerloop.IO_context()),
    m_beaconcheck_timer(m_client_timerloop.IO_context()),
    m_thread_pool(1),
    m_event_condition(std::make_shared<std::condition_variable>()),
    m_api(api)
{
    // Set up wake conditions
    m_session->session_events()->set_wake_condition(m_event_condition);
    m_session->compute_events()->set_wake_condition(m_event_condition);
    m_session->synchronisable_events()->set_wake_condition(m_event_condition);
    m_session->hierarchy()->hierarchy_events()->set_wake_condition(m_event_condition);
    m_session->hierarchy()->synchronisable_events()->set_wake_condition(m_event_condition);

    ZstEventDispatcher<ZstConnectionAdaptor>::set_wake_condition(m_event_condition);
    m_plugins->plugin_events()->set_wake_condition(m_event_condition);

    //Set up beaconcheck timer
    m_beaconcheck_timer.expires_from_now(boost::posix_time::milliseconds(HEARTBEAT_DURATION));
    m_beaconcheck_timer.async_wait(boost::bind(&ZstClient::beaconcheck_timer, &m_beaconcheck_timer, this, boost::posix_time::milliseconds(HEARTBEAT_DURATION)));
    
    // Late setup of UDP transport since we need to pass the IO context
    m_udp_graph_transport = std::make_shared<ZstUDPGraphTransport>(m_client_timerloop.IO_context());
    m_tcp_graph_transport = std::make_shared<ZstTCPGraphTransport>();
}

ZstClient::~ZstClient() {
	destroy();
}

void ZstClient::destroy() {
    //Only need to call cleanup once
    if (m_is_ending || m_is_destroyed)
        return;
    set_init_completed(false);

    //Let stage know we are leaving
    if (is_connected_to_stage())
        leave_stage();

    //Since we've sent the leave request, we can flag that we are in the leave process
    set_is_ending(true);

    //Stop timers
    boost::system::error_code ec;
    m_beaconcheck_timer.cancel(ec);
    m_client_timerloop.IO_context().stop();
    m_client_timer_thread.interrupt();
    m_client_timer_thread.try_join_for(boost::chrono::milliseconds(100));

    //Stop transports
    m_service_broadcast_transport->destroy();
    m_client_transport->destroy();
    m_tcp_graph_transport->destroy();
    m_udp_graph_transport->destroy();

    //Stop threads
    m_thread_pool.join();
    m_thread_pool.stop();
    m_api = NULL;

    //Set last status flags
    set_is_ending(false);
    set_is_destroyed(true);

    //All done
    Log::net(Log::Level::notification, "Showtime library destroyed");
}

void ZstClient::init_client(const char* client_name, bool debug, uint16_t unreliable_port)
{
    if (m_is_ending || m_init_completed) {
        Log::net(Log::Level::notification, "Showtime already initialised");
        return;
    }

	// Set flags
    set_is_destroyed(false);
    set_is_ending(false);

	// Setup loggings
    auto log_events = ZstEventDispatcher<ZstLogAdaptor>::downcasted_shared_from_this<ZstEventDispatcher<ZstLogAdaptor>>();
    Log::init_logger("" , (debug) ? Log::Level::debug : Log::Level::notification, log_events);
    Log::net(Log::Level::notification, "Starting Showtime v{}", SHOWTIME_VERSION_STRING);

	// Set the name of this client
    m_client_name = client_name;

    //Todo: init IDs again after stage has responded
    ZstMsgIDManager::init(m_client_name.c_str(), m_client_name.size());

    //Create IO_context thread
    m_client_timer_thread = boost::thread(boost::ref(m_client_timerloop));
    m_client_timerloop.IO_context().restart();

	// Create session module
	m_session->init(client_name);
	m_session->register_entity(m_session->hierarchy()->get_local_performer());

    // Register client module to hierarchy to handle incoming events
    m_session->hierarchy()->hierarchy_events()->add_adaptor(ZstHierarchyAdaptor::downcasted_shared_from_this<ZstHierarchyAdaptor>());

    // Register client to plugin loader to handle loaded plugin events
    m_plugins->plugin_events()->add_adaptor(ZstPluginAdaptor::downcasted_shared_from_this<ZstPluginAdaptor>());

	// Register client transport adaptors to session
	m_session->stage_events()->add_adaptor(m_client_transport);
    std::static_pointer_cast<ZstClientHierarchy>(m_session->hierarchy())->stage_events()->add_adaptor(m_client_transport);

    //Setup adaptors to let transports communicate with client modules
    m_client_transport->init();
    m_client_transport->msg_events()->add_adaptor(ZstStageTransportAdaptor::downcasted_shared_from_this< ZstStageTransportAdaptor>());
    m_client_transport->msg_events()->add_adaptor(std::static_pointer_cast<ZstStageTransportAdaptor>(m_session));
    m_client_transport->msg_events()->add_adaptor(std::static_pointer_cast<ZstStageTransportAdaptor>(std::static_pointer_cast<ZstClientHierarchy>(m_session->hierarchy())));

    //Setup graph transports
    m_tcp_graph_transport->init();
    m_tcp_graph_transport->bind("");
    m_tcp_graph_transport->msg_events()->add_adaptor(ZstGraphTransportAdaptor::downcasted_shared_from_this< ZstGraphTransportAdaptor>());
    m_tcp_graph_transport->msg_events()->add_adaptor(std::static_pointer_cast<ZstGraphTransportAdaptor>(m_session));
   
    m_udp_graph_transport->set_port(unreliable_port);
    m_udp_graph_transport->init();
    m_udp_graph_transport->bind("");
    m_udp_graph_transport->msg_events()->add_adaptor(ZstGraphTransportAdaptor::downcasted_shared_from_this< ZstGraphTransportAdaptor>());
    m_udp_graph_transport->msg_events()->add_adaptor(std::static_pointer_cast<ZstGraphTransportAdaptor>(m_session));

    //Stage discovery beacon
    m_service_broadcast_transport->init(STAGE_DISCOVERY_PORT);
    m_service_broadcast_transport->start_listening();
    m_service_broadcast_transport->msg_events()->add_adaptor(ZstServiceDiscoveryAdaptor::downcasted_shared_from_this< ZstServiceDiscoveryAdaptor>());
    
    //Load plugins
    m_plugins->load();

    //Init completed
    set_init_completed(true);

    //Finally, process events so submodules can talk to each other at the start
    process_events();
}

void ZstClient::init_file_logging(const char* log_file_path)
{
    Log::init_file_logging(log_file_path);
}

void ZstClient::process_events()
{
    process_events_imp(false);
}

void ZstClient::process_events_imp(bool block)
{
    if (!is_init_complete() || m_is_destroyed || m_is_ending) {
        Log::net(Log::Level::debug, "Can't process events until the library is ready");
        return;
    }

    if (block) {
        auto lock = std::unique_lock(m_mtx);
        m_event_condition->wait(lock);
    }

    m_plugins->process_events();
    m_session->process_events();
    ZstEventDispatcher<ZstConnectionAdaptor>::process_events();
    ZstEventDispatcher<ZstLogAdaptor>::process_events();
}

void ZstClient::flush()
{
    m_session->flush_events();
    m_plugins->flush_events();
}

// -----------------------
// Stage adaptor overrides
// -----------------------

void ZstClient::on_receive_msg(const std::shared_ptr<ZstStageMessage>& msg)
{
    switch (msg->buffer()->content_type()) {
    case Content_ClientGraphHandshakeStart:
        start_connection_broadcast_handler(msg);
        break;
    case Content_ClientGraphHandshakeStop:
        stop_connection_broadcast_handler(msg);//->buffer()->content_as_ClientGraphHandshakeStop());
        break;
    case Content_ClientGraphHandshakeListen:
        listen_to_client_handler(msg); // ->buffer()->content_as_ClientGraphHandshakeListen(), msg->id());
        break;
    case Content_ServerStatusMessage:
        server_status_handler(msg); // ->buffer()->content_as_ServerStatusMessage());
        break;
    default:
        break;
    }
}

void ZstClient::on_receive_msg(const std::shared_ptr<ZstPerformanceMessage>& msg)
{
    connection_handshake_handler(msg);
}

void ZstClient::on_receive_msg(const std::shared_ptr<ZstServerBeaconMessage>& msg)
{
	server_discovery_handler(msg);
}

void ZstClient::start_connection_broadcast_handler(const std::shared_ptr<showtime::ZstStageMessage>& msg)
{
    // Build request message that will be sent to the peer
    auto request = msg->buffer()->content_as_ClientGraphHandshakeStart();
    auto remote_client_path = request->receiver_URI();
    if (!remote_client_path) {
        Log::net(Log::Level::warn, "Received malformed ClientGraphHandshakeStart message");
        return;
    }

    // Create new mapping of our remote client to our broadcast timers
    ZstURI remote_path(remote_client_path->str().c_str());

    // Get addresses and connection properties from message
    auto connection_type = msg->buffer()->content_as_ClientGraphHandshakeStart()->connection_type();
    auto receiver_address = msg->buffer()->content_as_ClientGraphHandshakeStart()->receiver_address()->str();
    auto receiver_public_address = msg->buffer()->content_as_ClientGraphHandshakeStart()->receiver_public_address()->str();

    // Coroutine to wait for timers to finish
    start_connection_handshake(remote_path, std::vector<std::string>{receiver_address, receiver_public_address}, MAX_HANDSHAKE_MESSAGES, connection_type);
}

void ZstClient::connection_handshake_handler(std::shared_ptr<ZstPerformanceMessage> msg)
{
    if (msg->buffer()->value()->values_type() != PlugValueData_PlugHandshake) {
        return;
    }

    if(m_pending_peer_connections.size() < 1)
        return;
    
    ZstURI output_path(msg->buffer()->sender()->c_str(), msg->buffer()->sender()->size());
    if (m_pending_peer_connections.find(output_path) != m_pending_peer_connections.end()) {
        Log::net(Log::Level::debug, "Received connection handshake. Msg id {}", boost::to_string(m_pending_peer_connections[output_path]));
        
        // Let server know we received the handshake
        ZstTransportArgs args;
        args.msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;
        args.msg_ID = m_pending_peer_connections[output_path];
        auto builder = std::make_shared< FlatBufferBuilder>();
        auto ok_msg = CreateSignalMessage(*builder, Signal_OK);
        this->m_client_transport->send_msg(Content_SignalMessage, ok_msg.Union(), builder, args);

        // Clear the handshake
        m_pending_peer_connections.erase(output_path);

        // Stop any active handshake timers we're using to send messages for the NAT punchthrough
        std::string remote_address = msg->buffer()->value()->values_as_PlugHandshake()->sender_address()->str();
        auto handshake_it = std::find_if(
            this->m_endpoint_handshakes.begin(),
            this->m_endpoint_handshakes.end(),
            [remote_address](const EndpointHandshakeInfo& info) {return info.address == remote_address; }
        );      
        if (handshake_it != m_endpoint_handshakes.end()) {
            auto handshake = m_endpoint_handshakes.extract(handshake_it);
            handshake.value().success = true;
            m_endpoint_handshakes.insert(std::move(handshake));
        }
    }
}

void ZstClient::server_status_handler(const std::shared_ptr<ZstStageMessage>& request)
{
    auto server_status = request->buffer()->content_as_ServerStatusMessage();
    Log::net(Log::Level::debug, "Received a server status update: {}", EnumNameServerStatus(server_status->status()));
    if (server_status->status() == ServerStatus_QUIT) {
        leave_stage_complete(ZstTransportRequestBehaviour::PUBLISH);
    }
}


// -------------
// Client join
// -------------

ZstServerAddress ZstClient::get_discovered_server(const std::string& server_name) const
{
    auto server_address = std::find_if(
        this->get_discovered_servers().begin(), 
        this->get_discovered_servers().end(), 
        [server_name](const ZstServerAddress& server) {return server.name == server_name;}
    );
    return (server_address != this->get_discovered_servers().end()) ? *server_address : ZstServerAddress();
}

void ZstClient::auto_join_stage(const std::string& name, const ZstTransportRequestBehaviour& sendtype)
{
    m_auto_join_stage = true;

    // If the server beacon as already been received, join immediately
    for (auto server : m_server_beacons) {
        if (server.name == name) {
            join_stage(server, sendtype);
            return;
        }
    }

    // Create a future that let us wait until a particular server beacon arrives
    m_auto_join_stage_requests[name] = std::promise<ZstMessageResponse>();

    if (sendtype == ZstTransportRequestBehaviour::ASYNC_REPLY) {
        boost::asio::post(m_thread_pool, [this, name, sendtype]() {
            this->join_on_beacon(name, sendtype);
        });
    }
    else if (sendtype == ZstTransportRequestBehaviour::SYNC_REPLY) {
        this->join_on_beacon(name, sendtype);
    }
}

void ZstClient::join_on_beacon(const std::string & server_name, ZstTransportRequestBehaviour sendtype) {
    // Wait to receive a beacon
    auto response_future = m_auto_join_stage_requests[server_name].get_future();
    auto response_status = response_future.wait_for(std::chrono::milliseconds(STAGE_TIMEOUT));
    
    // Handle timeouts
    if (response_status == std::future_status::timeout) {
        Log::net(Log::Level::warn, "Didn't receive a beacon from server {}. Can't join.", server_name);
        m_auto_join_stage_requests.erase(m_auto_join_stage_requests.find(server_name));
        return;
    }

    try {
        auto response = response_future.get();

        // Join stage using our new beacon
        auto address = server_beacon_to_address(std::static_pointer_cast<ZstServerBeaconMessage>(response.response));
        this->join_stage(address, sendtype);

        //Since we took responsibility for this beacon message we need to clean it up
        response.response->owning_transport()->release_owned_message(response.response);
        m_auto_join_stage_requests.erase(m_auto_join_stage_requests.find(server_name));

    } catch (std::future_error e) {
        Log::net(Log::Level::error, "Promise failed with error {}", e.what());
    }
}


void ZstClient::join_stage(const ZstServerAddress& stage_address, const ZstTransportRequestBehaviour& sendtype) {

    if (!m_init_completed) {
        Log::net(Log::Level::error, "Can't join the stage until the library has been initialised");
        return;
    }

    if (m_is_connecting) {
        Log::net(Log::Level::error, "Can't connect to stage, already connecting");
        return;
    }

    if (m_is_connecting || m_connected_to_stage) {
        Log::net(Log::Level::error, "Can't connect to stage, already connected");
        return;
    }
    set_is_connecting(true);

    Log::net(Log::Level::notification, "Connecting to stage {}", stage_address.address);
    m_client_transport->connect(stage_address.address);

    // Get our local/public addresses
    std::string unreliable_graph_addr = m_udp_graph_transport->get_graph_in_address();
    std::string unreliable_public_graph_addr = m_udp_graph_transport->getPublicIPAddress(STUNServer{ STUN_SERVER, 40006, m_udp_graph_transport->get_port() }); //m_udp_graph_transport->get_incoming_port()
    Log::net(Log::Level::debug, "UDP public address: {}", unreliable_public_graph_addr);

    std::string reliable_graph_addr = m_tcp_graph_transport->get_graph_out_address();
    std::string reliable_public_graph_addr = m_tcp_graph_transport->getPublicIPAddress(STUNServer{ STUN_SERVER, 40006, m_tcp_graph_transport->get_port() });
    Log::net(Log::Level::debug, "TCP public address: {}", reliable_public_graph_addr);

    // Keep a connection to the STUN server open so we can send keepalive messages
    //m_udp_graph_transport->connect(fmt::format("{}:{}", STUN_SERVER, 40006));
    m_udp_graph_transport->listen();
    m_tcp_graph_transport->listen();

    //Activate any child entities and factories that were added to the root performer already
    ZstPerformer* root = session()->hierarchy()->get_local_performer();

    //Construct transport args
    auto builder = std::make_shared< FlatBufferBuilder>();
    ZstTransportArgs args;
    args.msg_send_behaviour = sendtype;
    args.on_recv_response = [this, stage_address](ZstMessageResponse response) {
        if (!ZstStageTransport::verify_signal(response.response, Signal_OK, ""))
            return;
        this->join_stage_complete(stage_address, response);
    };

    Offset<Performer> root_offset = root->serialize(*builder);
    auto join_msg = CreateClientJoinRequest(*builder, 
        root_offset, 
        builder->CreateString(reliable_graph_addr), 
        builder->CreateString(reliable_public_graph_addr),
        builder->CreateString(unreliable_graph_addr), 
        builder->CreateString(unreliable_public_graph_addr)
    );

    m_client_transport->send_msg(Content_ClientJoinRequest, join_msg.Union(), builder, args);
}

void ZstClient::server_discovery_handler(const std::shared_ptr<ZstServerBeaconMessage>& msg)
{
    // Make a server address to hold our server name/address pair
    ZstServerAddress server = server_beacon_to_address(msg);
    
    if (m_server_beacons.find(server) != m_server_beacons.end()) {
        // Reset beacon timeout
        m_server_beacon_timestamps[server] = std::chrono::system_clock::now();
        return;
    }

    // Add server to list of discovered servers if the beacon hasn't been seen before
    Log::net(Log::Level::debug, "Received new server beacon: {} {}", server.name, server.address);

    // Store server beacon
    m_server_beacons.insert(server);
    m_server_beacon_timestamps.emplace(server, std::chrono::system_clock::now());
    ZstEventDispatcher<ZstConnectionAdaptor>::defer([this, server](ZstConnectionAdaptor* adaptor) {
        auto server_address = std::make_shared<ZstServerAddress>(server);
        adaptor->on_server_discovered(this->m_api, server_address.get());
    });

    // Handle connecting to the stage automatically
    if (m_auto_join_stage && !is_connected_to_stage()) {
        auto server_search_request = m_auto_join_stage_requests.find(server.name);
        if (server_search_request != m_auto_join_stage_requests.end()) {
            // Whilst the beacon is not technically a response to a message we have sent, we treat it as one
            // so it doesn't get reset until after the promise gets satisfied
            msg->set_has_promise();
            server_search_request->second.set_value(ZstMessageResponse{ msg, ZstTransportRequestBehaviour::PUBLISH });
        }
    }
}

ZstServerAddress ZstClient::server_beacon_to_address(const std::shared_ptr<ZstServerBeaconMessage>& msg) {
    return ZstServerAddress{ msg->buffer()->name()->str(), std::format("{}:{}", msg->address(), msg->buffer()->port()) };
}

const ZstServerList& ZstClient::get_discovered_servers() const
{
    return m_server_beacons;
}


void ZstClient::join_stage_complete(const ZstServerAddress& server_address, ZstMessageResponse response)
{
    set_is_connecting(false);
    set_connected_to_stage(true);
    m_connected_server = server_address;

    if (!ZstStageTransport::verify_signal(response.response, Signal_OK, "Join stage")) {
        leave_stage_complete(response.send_behaviour);
        return;
    }

    Log::net(Log::Level::notification, "Connection to server established");

    //Add local entities to entity lookup and attach adaptors only if we've connected to the stage
    ZstEntityBundle bundle;
    m_session->hierarchy()->get_local_performer()->get_child_entities(&bundle, true);
    for (auto c : bundle) {
        c->add_adaptor(static_cast<std::shared_ptr< ZstSynchronisableAdaptor>>(m_session->hierarchy()));
        c->add_adaptor(static_cast<std::shared_ptr<ZstEntityAdaptor> >(m_session->hierarchy()));
        synchronisable_set_activating(c);
        synchronisable_enqueue_activation(c);
    }

    //Enqueue connection events
    m_session->dispatch_connected_to_stage();

    //Set up heartbeat timer
    m_heartbeat_timer.expires_from_now(boost::posix_time::milliseconds(HEARTBEAT_DURATION));
    m_heartbeat_timer.async_wait(boost::bind(&ZstClient::heartbeat_timer, &m_heartbeat_timer, this, boost::posix_time::milliseconds(HEARTBEAT_DURATION)));

    //Ask the stage to send us the current session
    synchronise_graph(response.send_behaviour);

    // Activate all child entities that were added before we joined
    bundle.clear();
    session()->hierarchy()->get_local_performer()->get_child_entities(&bundle, false, true);
    for (auto c : bundle) {
        Log::net(Log::Level::notification, "Post-join activating child entity {}", c->URI().path());
        session()->hierarchy()->activate_entity(c, response.send_behaviour);
    }

    //Enqueue connection events
    m_session->dispatch_connected_to_stage();
    ZstEventDispatcher<ZstConnectionAdaptor>::defer([this, server_address](ZstConnectionAdaptor* adaptor) {
        auto address = std::make_shared<ZstServerAddress>(server_address);
        adaptor->on_connected_to_server(this->m_api, address.get());
    });

    //If we are sync, we can dispatch events immediately
    if (response.send_behaviour == ZstTransportRequestBehaviour::SYNC_REPLY)
        process_events();
}

void ZstClient::synchronise_graph(const ZstTransportRequestBehaviour& sendtype)
{
    Log::net(Log::Level::notification, "Requesting graph snapshot");

    //Build message
    ZstTransportArgs args;
    args.msg_send_behaviour = sendtype;
    args.on_recv_response = [this](ZstMessageResponse response) {
        if (!ZstStageTransport::verify_signal(response.response, Signal_OK, "Synchronise client with server")) {
            return;
        }
        this->synchronise_graph_complete(response);
    };

    //Send message
    auto builder = std::make_shared< FlatBufferBuilder>();
    auto sync_signal = CreateSignalMessage(*builder, Signal_CLIENT_SYNC);
    m_client_transport->send_msg(Content_SignalMessage, sync_signal.Union(), builder, args);
}

void ZstClient::synchronise_graph_complete(ZstMessageResponse response)
{
    Log::net(Log::Level::notification, "Graph sync for {} completed", session()->hierarchy()->get_local_performer()->URI().path());
    ZstEventDispatcher<ZstConnectionAdaptor>::defer([this](ZstConnectionAdaptor* adp) {
        auto server_address = std::make_shared<ZstServerAddress>(this->connected_server());
        adp->on_synchronised_graph(this->m_api, server_address.get());
    });
}

void ZstClient::leave_stage()
{
    if (is_connected_to_stage()) {
        Log::net(Log::Level::notification, "Leaving stage");

        //Set flags early to avoid double leaving shenanigans
        this->set_is_connecting(false);

        ZstTransportArgs args;
        args.msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;
        auto builder = std::make_shared< FlatBufferBuilder>();
        auto leave_msg_offset = CreateClientLeaveRequest(*builder, builder->CreateString(session()->hierarchy()->get_local_performer()->URI().path()), ClientLeaveReason_QUIT);
        m_client_transport->send_msg(Content_ClientLeaveRequest, leave_msg_offset.Union(), builder, args);
    }
    else {
        Log::net(Log::Level::debug, "Not connected to stage. Skipping to cleanup. {}");
    }
    
    // Don't wait for server response
    this->leave_stage_complete(ZstTransportRequestBehaviour::PUBLISH);
    this->process_events();
}

void ZstClient::leave_stage_complete(ZstTransportRequestBehaviour sendtype)
{
    if (!is_connected_to_stage())
        return;

    //Set stage as disconnected again - just to make sure
    set_connected_to_stage(false);
    set_is_connecting(false);

	//Disconnect sockets and timers
	boost::system::error_code ec;
	m_heartbeat_timer.cancel(ec);
	//m_heartbeat_timer.wait();
	Log::net(Log::Level::debug, "Timer cancel status: {}", ec.message());
	if (m_client_transport)
		m_client_transport->disconnect();

	if (m_tcp_graph_transport)
		m_tcp_graph_transport->disconnect();

	if (m_udp_graph_transport)
		m_udp_graph_transport->disconnect();

    //Mark all locally registered entites as deactivated
    ZstEntityBundle bundle;
    m_session->hierarchy()->get_local_performer()->get_child_entities(&bundle, true);
    for (auto c : bundle) {
		synchronisable_enqueue_deactivation(c);
    }

	//Remove entities immediately
    if(sendtype == ZstTransportRequestBehaviour::SYNC_REPLY)
	    process_events();

    //Enqueue event for adaptors
    m_session->dispatch_disconnected_from_stage();
    ZstEventDispatcher<ZstConnectionAdaptor>::defer([this](ZstConnectionAdaptor* adaptor) {
        if (m_api) {
            auto server_address = std::make_shared<ZstServerAddress>(this->connected_server());
            adaptor->on_disconnected_from_server(this->m_api, server_address.get());
        }
    });

    if (sendtype == ZstTransportRequestBehaviour::SYNC_REPLY)
        process_events();

    m_connected_server = ZstServerAddress();
}

const ZstServerAddress& ZstClient::connected_server()
{
    return m_connected_server;
}

bool ZstClient::is_connected_to_stage()
{
    return m_connected_to_stage;
}

bool ZstClient::is_connecting_to_stage()
{
    return m_is_connecting;
}

bool ZstClient::is_init_complete() {
    return m_init_completed;
}

long ZstClient::ping()
{
    return m_ping;
}

void ZstClient::heartbeat_timer(boost::asio::deadline_timer* t, ZstClient* client, boost::posix_time::milliseconds duration) {
    if (!client->is_connected_to_stage())
        return;

    //Build message
    std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
    ZstTransportArgs args;
    args.msg_send_behaviour = ZstTransportRequestBehaviour::ASYNC_REPLY;
    args.on_recv_response = [client, start](ZstMessageResponse response) {
        if (!client)
            return;

        if (!ZstStageTransport::verify_signal(response.response, Signal_OK, "Heartbeat ack")) {
            client->leave_stage_complete(ZstTransportRequestBehaviour::PUBLISH);
            return;
        }

        std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
        auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        client->m_ping = static_cast<long>(delta.count());
    };

    //Send message
    auto builder = std::make_shared< FlatBufferBuilder>();
    auto heartbeat_signal = CreateSignalMessage(*builder, Signal_CLIENT_HEARTBEAT);
    client->m_client_transport->send_msg(Content_SignalMessage, heartbeat_signal.Union(), builder, args);

    // Send keepalive message through UDP graph to keep punchthrough in NAT open
    args = ZstTransportArgs();
    args.msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;
    builder = std::make_shared<FlatBufferBuilder>();
    auto plugval_offset = CreatePlugValue(*builder, PlugValueData_PlugKeepalive, CreatePlugKeepalive(*builder).Union());
    auto from = client->session()->hierarchy()->get_local_performer()->URI();
    auto conn_msg = CreateGraphMessage(*builder, builder->CreateString(from.path(), from.full_size()), plugval_offset);
    //client->m_udp_graph_transport->send_msg(conn_msg, builder, args);

    //Loop timer
    t->expires_at(t->expires_at() + duration);
    t->async_wait(boost::bind(&ZstClient::heartbeat_timer, t, client, duration));
}

void ZstClient::beaconcheck_timer(boost::asio::deadline_timer* t, ZstClient* client, boost::posix_time::milliseconds duration)
{
    auto now = std::chrono::system_clock::now();

    std::vector<ZstServerAddress> removed_beacons;
    for(auto server : client->m_server_beacon_timestamps){
        auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - server.second);
        if (delta > std::chrono::milliseconds(STAGE_TIMEOUT)) {
            removed_beacons.push_back(server.first);
        }
    }
    for (auto server : removed_beacons) {
        client->lost_server_beacon(server);
    }

    t->expires_at(t->expires_at() + duration);
    t->async_wait(boost::bind(&ZstClient::beaconcheck_timer, t, client, duration));
}

void ZstClient::lost_server_beacon(const ZstServerAddress& server)
{   
    // Remove server address
    auto server_it = m_server_beacons.find(server);
    if(server_it != m_server_beacons.end())
        m_server_beacons.erase(m_server_beacons.find(server));

    // Remove server timestamp
    auto server_timestamp_it = m_server_beacon_timestamps.find(server);
    if (server_timestamp_it != m_server_beacon_timestamps.end())
        m_server_beacon_timestamps.erase(m_server_beacon_timestamps.find(server));

    // Broadcast events
    ZstEventDispatcher<ZstConnectionAdaptor>::defer([this, server](ZstConnectionAdaptor* adaptor) {
        auto server_address = std::make_shared<ZstServerAddress>(server);
        adaptor->on_server_lost(m_api, std::make_shared<ZstServerAddress>(server).get());
    });
}

void ZstClient::auto_join_stage_complete()
{
}

void ZstClient::start_connection_handshake(const ZstURI& remote_client_path, const std::vector<std::string>& remote_client_addresses, size_t total_messages, showtime::ConnectionType connection_type)
{       
   std::shared_ptr<ZstGraphTransport> transport = (connection_type == ConnectionType::ConnectionType_UNRELIABLE)  
       ? std::static_pointer_cast<ZstGraphTransport>(m_udp_graph_transport) 
       : std::static_pointer_cast<ZstGraphTransport>(m_tcp_graph_transport);

    // Coroutine to failover from private to public addresses
   spawn(m_client_timerloop.IO_context(), [this, remote_client_path, remote_client_addresses, total_messages, transport, connection_type](boost::asio::yield_context yield) {
        for (auto address : remote_client_addresses) 
        {
            bool success = false;

            // Only UDP transports connect (Sender->Receiver)
            if (connection_type == ConnectionType::ConnectionType_UNRELIABLE) {
                transport->connect(address);
            }

            // Hold onto the status of this handshake
            m_endpoint_handshakes.emplace(EndpointHandshakeInfo{ remote_client_path, address, false });

            //Log::net(Log::Level::debug, "Sending handshake to endpoint {}", address);

            for (size_t msg_count = 0; msg_count < total_messages; ++msg_count)
            {
                Log::net(Log::Level::debug, "Sending handshake to endpoint {}", address);

                // Check if this connection was successful
                auto connection_status = std::find_if(
                    this->m_endpoint_handshakes.begin(), 
                    this->m_endpoint_handshakes.end(), 
                    [address](const EndpointHandshakeInfo& info) {return info.address == address; }
                );
                if (connection_status != this->m_endpoint_handshakes.end()){
                    if (connection_status->success) {
                        success = connection_status->success;
                        Log::net(Log::Level::debug, "Endpoint {} successfully received a message from {}", address, session()->hierarchy()->get_local_performer()->URI().path());
                        break;
                    }
                }

                // Send a handshake to the remote peer
                send_connection_handshake(
                    session()->hierarchy()->get_local_performer()->URI(),
                    address,
                    transport
                );

                //Wait for a bit before we send another message
                auto endpoint_timer = boost::asio::steady_timer(m_client_timerloop.IO_context());
                endpoint_timer.expires_after(30ms);
                endpoint_timer.async_wait(yield);
            }

            if (success)
                break;
            else {
                transport->disconnect(address);
                Log::net(Log::Level::warn, "No handshake response from endpoint {}", address);
            }
        }
    });
}

void ZstClient::send_connection_handshake(const ZstURI& from, const std::string& address, std::shared_ptr<ZstGraphTransport> transport)
{
    ZstTransportArgs args;
    args.msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;

    auto builder = std::make_shared<FlatBufferBuilder>();
    auto plugval_offset = CreatePlugValue(*builder, PlugValueData_PlugHandshake, CreatePlugHandshake(*builder, builder->CreateString(address.c_str(), address.size())).Union());
    auto conn_msg = CreateGraphMessage(*builder, builder->CreateString(from.path(), from.full_size()), plugval_offset);
    transport->send_msg(conn_msg, builder, args);
}

void ZstClient::stop_connection_broadcast_handler(const std::shared_ptr<ZstStageMessage>& msg)
{
    auto request = msg->buffer()->content_as_ClientGraphHandshakeStop();
    auto remote_client_path = ZstURI(request->receiver_URI()->c_str());
    ZstPerformer* remote_client = dynamic_cast<ZstPerformer*>(session()->hierarchy()->find_entity(remote_client_path));
    Log::net(Log::Level::debug, "Stopping peer handshake broadcast to {}", remote_client->URI().path());

    if (!remote_client) {
        Log::net(Log::Level::error, "Could not find performer {}", remote_client_path.path());
        return;
    }

    // Stop handshake broadcasts by setting the success flag
    for(auto handshake_it : m_endpoint_handshakes){
        if (handshake_it.destination == remote_client_path) {
            auto handshake = m_endpoint_handshakes.extract(handshake_it);
            handshake.value().success = true;
            m_endpoint_handshakes.insert(std::move(handshake));
        }
    }
}

void ZstClient::listen_to_client_handler(const std::shared_ptr<ZstStageMessage>& msg)
{
    auto request = msg->buffer()->content_as_ClientGraphHandshakeListen();
    auto output_path = ZstURI(request->sender_URI()->c_str(), request->sender_URI()->size());
    auto receiver_address = request->sender_address()->str();
    auto receiver_public_address = request->sender_public_address()->str();
    auto connection_type = request->connection_type();

    // Only UDP transports connect (Sender->Receiver)
    if (connection_type == ConnectionType::ConnectionType_RELIABLE) {
        m_tcp_graph_transport->connect(receiver_address);
        m_tcp_graph_transport->connect(receiver_public_address);
    }

    // Send keepalive message to the remote client so that our Port-Restricted cone NAT will allow incoming packets from our address
    start_connection_handshake(output_path, std::vector<std::string>{receiver_address, receiver_public_address}, MAX_HANDSHAKE_MESSAGES*2, connection_type);
    
    // TODO: This needs to happen here when using TCP to detect connection status
    m_pending_peer_connections[output_path] = msg->id();
}

std::shared_ptr<ZstClientSession> ZstClient::session()
{
    return m_session;
}

std::shared_ptr<ZstPluginLoader> ZstClient::plugins()
{
	return m_plugins;
}

void ZstClient::set_is_ending(bool value)
{
    m_is_ending = value;
}

void ZstClient::set_is_destroyed(bool value)
{
    m_is_destroyed = value;
}

void ZstClient::set_init_completed(bool value)
{
    m_init_completed = value;
}

void ZstClient::set_connected_to_stage(bool value)
{
    m_connected_to_stage = value;
}

void ZstClient::set_is_connecting(bool value)
{
    m_is_connecting = value;
}

void ZstClient::on_entity_arriving(ZstEntityBase* entity)
{
    ZstEntityBundle bundle;
    entity->get_child_entities(&bundle, true, true);

    for (auto child : bundle) {
        // Arriving output plugs need to register the graph transport so that they can dispatch messages
        if (child->entity_type() == ZstEntityType::PLUG) {
            init_arriving_plug(static_cast<ZstPlug*>(child));
        }
    }
}

void ZstClient::on_performer_arriving(ZstPerformer* performer)
{
    on_entity_arriving(performer);
}

void ZstClient::on_plugin_loaded(ZstPlugin* plugin)
{
    ZstEntityFactoryBundle bundle;
    plugin->get_factories(bundle);
    for (auto f : bundle) {
        // Register each factory in the plugin with the performance
        this->session()->hierarchy()->get_local_performer()->add_child(f);
        if(is_connected_to_stage())
            this->session()->hierarchy()->activate_entity(f, ZstTransportRequestBehaviour::ASYNC_REPLY);
    }
}

void ZstClient::init_arriving_plug(ZstPlug* plug)
{
    ZstOutputPlug* output_plug = dynamic_cast<ZstOutputPlug*>(plug);
    if (output_plug) {
        std::shared_ptr<ZstGraphTransport> transport = NULL;
        if (output_plug->is_reliable()) {
            transport = std::static_pointer_cast<ZstGraphTransport>(m_tcp_graph_transport);
        }
#ifdef ZST_BUILD_DRAFT_API
        else {
            transport = std::static_pointer_cast<ZstGraphTransport>(m_udp_graph_transport);

        }
#endif
        // Setup plug as fireable if we own it
        if (session()->hierarchy()->get_local_performer()->URI() == plug->URI().first()) {
            output_plug_set_can_fire(output_plug, true);
        }

        // Attach plug transport so output plugs can send messages to the graph
        output_plug_set_transport(output_plug, std::static_pointer_cast<ZstGraphTransportAdaptor>(transport));
    }
}

}
