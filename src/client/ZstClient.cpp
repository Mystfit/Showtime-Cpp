#include "ZstClient.h"
#include <boost/uuid/uuid_io.hpp>

using namespace flatbuffers;

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

    //Transports
    m_tcp_graph_transport(std::make_shared<ZstTCPGraphTransport>()),
#ifdef ZST_BUILD_DRAFT_API
    m_udp_graph_transport(std::make_shared<ZstUDPGraphTransport>());
#endif
    m_client_transport(std::make_shared<ZstZMQClientTransport>()),

    //Timers
    m_beaconcheck_timer(m_client_timerloop.IO_context()),
    m_heartbeat_timer(m_client_timerloop.IO_context()),
    m_event_condition(std::make_shared<ZstSemaphore>()),
    m_thread_pool(1),
    m_api(api)
{
    m_session->session_events()->set_wake_condition(m_event_condition);
    m_session->hierarchy()->hierarchy_events()->set_wake_condition(m_event_condition);
    ZstEventDispatcher< std::shared_ptr<ZstConnectionAdaptor> >::set_wake_condition(m_event_condition);

    //Set up beaconcheck timer
    m_beaconcheck_timer.expires_from_now(boost::posix_time::milliseconds(HEARTBEAT_DURATION));
    m_beaconcheck_timer.async_wait(boost::bind(&ZstClient::beaconcheck_timer, &m_beaconcheck_timer, this, boost::posix_time::milliseconds(HEARTBEAT_DURATION)));
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
    m_client_timer_thread.join();

    //Stop transports
    m_service_broadcast_transport->destroy();
    m_client_transport->destroy();
    m_tcp_graph_transport->destroy();
#ifdef ZST_BUILD_DRAFT_API
    m_udp_graph_transport->destroy();
#endif

    //Stop threads
    m_thread_pool.join();
    m_thread_pool.stop();
    m_api = NULL;

    //Set last status flags
    set_is_ending(false);
    set_is_destroyed(true);

    //All done
    ZstLog::net(LogLevel::notification, "Showtime library destroyed");
}

void ZstClient::init_client(const char* client_name, bool debug)
{
    if (m_is_ending || m_init_completed) {
        ZstLog::net(LogLevel::notification, "Showtime already initialised");
        return;
    }

	// Set flags
    set_is_destroyed(false);
    set_is_ending(false);

	// Setup logging
    LogLevel level = LogLevel::notification;
    if (debug)
        level = LogLevel::debug;
    ZstLog::init_logger(client_name, level);
    ZstLog::net(LogLevel::notification, "Starting Showtime v{}", SHOWTIME_VERSION_STRING);

	// Set the name of this client
    m_client_name = client_name;

    //Todo: init IDs again after stage has responded
    ZstMsgIDManager::init(m_client_name.c_str(), m_client_name.size());

    //Create IO_context thread
    m_client_timer_thread = boost::thread(boost::ref(m_client_timerloop));
    m_connection_timers = ZstConnectionTimerMapUnique(new ZstConnectionTimerMap());
    m_client_timerloop.IO_context().restart();

	// Create session module
	m_session->init(client_name);
	m_session->register_entity(m_session->hierarchy()->get_local_performer());

    // Register client module to hierarchy to handle incoming events
    m_session->hierarchy()->hierarchy_events()->add_adaptor(ZstHierarchyAdaptor::downcasted_shared_from_this<ZstHierarchyAdaptor>());

	// Register client transport adaptors to session
	m_session->stage_events()->add_adaptor(m_client_transport);
    std::static_pointer_cast<ZstClientHierarchy>(m_session->hierarchy())->stage_events()->add_adaptor(m_client_transport);

    //Setup adaptors to let transports communicate with client modules
    m_client_transport->init();
    m_client_transport->msg_events()->add_adaptor(ZstStageTransportAdaptor::downcasted_shared_from_this< ZstStageTransportAdaptor>());
    m_client_transport->msg_events()->add_adaptor(std::static_pointer_cast<ZstStageTransportAdaptor>(m_session));
    m_client_transport->msg_events()->add_adaptor(std::static_pointer_cast<ZstStageTransportAdaptor>(std::static_pointer_cast<ZstClientHierarchy>(m_session->hierarchy())));

    //Setup adaptors to receive graph messages
    m_tcp_graph_transport->init();
    m_tcp_graph_transport->msg_events()->add_adaptor(ZstStageTransportAdaptor::downcasted_shared_from_this< ZstStageTransportAdaptor>());
    m_tcp_graph_transport->msg_events()->add_adaptor(std::static_pointer_cast<ZstStageTransportAdaptor>(m_session));

#ifdef ZST_BUILD_DRAFT_API
    m_udp_graph_transport->init();
    m_udp_graph_transport->msg_events()->add_adaptor(ZstStageTransportAdaptor::downcasted_shared_from_this< ZstStageTransportAdaptor>());
    m_udp_graph_transport->msg_events()->add_adaptor(static_cast<ZstStageTransportAdaptor*>(m_session);
#endif

    //Stage discovery beacon
    m_service_broadcast_transport->init(STAGE_DISCOVERY_PORT);
    m_service_broadcast_transport->start_listening();
    m_service_broadcast_transport->msg_events()->add_adaptor(ZstStageTransportAdaptor::downcasted_shared_from_this< ZstStageTransportAdaptor>());

    //Init completed
    set_init_completed(true);
}

void ZstClient::init_file_logging(const char* log_file_path)
{
    ZstLog::init_file_logging(log_file_path);
}

void ZstClient::process_events()
{
    if (!is_init_complete() || m_is_destroyed || m_is_ending) {
        ZstLog::net(LogLevel::debug, "Can't process events until the library is ready");
        return;
    }
    m_session->process_events();
    ZstEventDispatcher< std::shared_ptr<ZstConnectionAdaptor> >::process_events();
}

void ZstClient::flush()
{
    m_session->flush_events();
}

// -----------------------
// Stage adaptor overrides
// -----------------------

void ZstClient::on_receive_msg(const std::shared_ptr<ZstStageMessage>& msg)
{
    switch (msg->buffer()->content_type()) {
    case Content_ClientGraphHandshakeStart:

#ifdef ZST_BUILD_DRAFT_API
        //sm_udp_graph_transport->connect(stage_msg->get_arg<std::string>(ZstMsgArg::GRAPH_UNRELIABLE_INPUT_ADDRESS));
#endif
        start_connection_broadcast_handler(msg);// buffer()->content_as_ClientGraphHandshakeStart());
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

// ------------------------------
// Performance dispatch overrides
// ------------------------------

void ZstClient::connection_handshake_handler(std::shared_ptr<ZstPerformanceMessage> msg)
{
    if (msg->buffer()->value()->values_type() != PlugValueData_PlugHandshake) {
        return;
    }

    if(m_pending_peer_connections.size() < 1)
        return;
    
    ZstURI output_path(msg->buffer()->sender()->c_str(), msg->buffer()->sender()->size());
    if (m_pending_peer_connections.find(output_path) != m_pending_peer_connections.end()) {
        ZstLog::net(LogLevel::debug, "Received connection handshake. Msg id {}", boost::to_string(m_pending_peer_connections[output_path]));
        
        auto id = m_pending_peer_connections[output_path];

        ZstTransportArgs args;
        args.msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;
        args.msg_ID = id;
        auto builder = std::make_shared< FlatBufferBuilder>();
        auto ok_msg = CreateSignalMessage(*builder, Signal_OK);
        this->m_client_transport->send_msg(Content_SignalMessage, ok_msg.Union(), builder, args);

        m_pending_peer_connections.erase(output_path);
    }
}

void ZstClient::server_status_handler(const std::shared_ptr<ZstStageMessage>& request)
{
    auto server_status = request->buffer()->content_as_ServerStatusMessage();
    ZstLog::net(LogLevel::debug, "Received a server status update: {}", EnumNameServerStatus(server_status->status()));
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
        ZstLog::net(LogLevel::warn, "Didn't receive a beacon from server {}. Can't join.", server_name);
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
        ZstLog::net(LogLevel::error, "Promise failed with error {}", e.what());
    }
}


void ZstClient::join_stage(const ZstServerAddress& stage_address, const ZstTransportRequestBehaviour& sendtype) {

    if (!m_init_completed) {
        ZstLog::net(LogLevel::error, "Can't join the stage until the library has been initialised");
        return;
    }

    if (m_is_connecting) {
        ZstLog::net(LogLevel::error, "Can't connect to stage, already connecting");
        return;
    }

    if (m_is_connecting || m_connected_to_stage) {
        ZstLog::net(LogLevel::error, "Can't connect to stage, already connected");
        return;
    }
    set_is_connecting(true);

    ZstLog::net(LogLevel::notification, "Connecting to stage {}", stage_address.address);
    m_client_transport->connect(stage_address.address);

    //Acquire our output graph address to send to the stage
    std::string reliable_graph_addr = m_tcp_graph_transport->get_graph_out_address();
#ifdef ZST_BUILD_DRAFT_API
    std::string unreliable_graph_addr = m_udp_graph_transport->get_graph_in_address();
#else
    std::string unreliable_graph_addr = "";
#endif

    //Activate any child entities and factories that were added to the root performer already
    ZstPerformer* root = session()->hierarchy()->get_local_performer();

    auto builder = std::make_shared< FlatBufferBuilder>();

    //Construct transport args
    ZstTransportArgs args;
    args.msg_send_behaviour = sendtype;
    args.on_recv_response = [this, stage_address](ZstMessageResponse response) {
        if (!ZstStageTransport::verify_signal(response.response, Signal_OK, ""))
            return;
        this->join_stage_complete(stage_address, response);
    };

    Offset<Performer> root_offset = root->serialize(*builder);
    auto join_msg = CreateClientJoinRequest(*builder, root_offset, builder->CreateString(reliable_graph_addr), builder->CreateString(unreliable_graph_addr));

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
    ZstLog::net(LogLevel::debug, "Received new server beacon: {} {}", server.name, server.address);

    // Store server beacon
    m_server_beacons.insert(server);
    m_server_beacon_timestamps.emplace(server, std::chrono::system_clock::now());
    ZstEventDispatcher<std::shared_ptr<ZstConnectionAdaptor>>::defer([this, server](std::shared_ptr<ZstConnectionAdaptor> adaptor) {
        adaptor->on_server_discovered(this->m_api, server);
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
    return ZstServerAddress(msg->buffer()->name()->str(), fmt::format("{}:{}", msg->address(), msg->buffer()->port()));
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

    ZstLog::net(LogLevel::notification, "Connection to server established");

    //Add local entities to entity lookup and attach adaptors only if we've connected to the stage
    ZstEntityBundle bundle;
    m_session->hierarchy()->get_local_performer()->get_factories(bundle);
    m_session->hierarchy()->get_local_performer()->get_child_entities(bundle, true);
    for (auto c : bundle) {
        c->synchronisable_events()->add_adaptor(static_cast<std::shared_ptr< ZstSynchronisableAdaptor> >(m_session->hierarchy()));
        c->entity_events()->add_adaptor(static_cast<std::shared_ptr<ZstEntityAdaptor> >(m_session->hierarchy()));
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

    //Enqueue connection events
    m_session->dispatch_connected_to_stage();
    ZstEventDispatcher<std::shared_ptr<ZstConnectionAdaptor> >::defer([this, server_address](std::shared_ptr<ZstConnectionAdaptor> adaptor) {
        adaptor->on_connected_to_stage(this->m_api, server_address);
    });

    //If we are sync, we can dispatch events immediately
    if (response.send_behaviour == ZstTransportRequestBehaviour::SYNC_REPLY)
        process_events();
}

void ZstClient::synchronise_graph(const ZstTransportRequestBehaviour& sendtype)
{
    ZstLog::net(LogLevel::notification, "Requesting graph snapshot");

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
    ZstLog::net(LogLevel::notification, "Graph sync for {} completed", session()->hierarchy()->get_local_performer()->URI().path());
    ZstEventDispatcher<std::shared_ptr<ZstConnectionAdaptor>>::defer([this](std::shared_ptr<ZstConnectionAdaptor> adp) {
        adp->on_synchronised_with_stage(this->m_api, this->connected_server());
    });
}

void ZstClient::leave_stage()
{
    if (is_connected_to_stage()) {
        ZstLog::net(LogLevel::notification, "Leaving stage");

        //Set flags early to avoid double leaving shenanigans
        this->set_is_connecting(false);

        ZstTransportArgs args;
        args.msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;
        auto builder = std::make_shared< FlatBufferBuilder>();
        auto leave_msg_offset = CreateClientLeaveRequest(*builder, builder->CreateString(session()->hierarchy()->get_local_performer()->URI().path()), ClientLeaveReason_QUIT);
        m_client_transport->send_msg(Content_ClientLeaveRequest, leave_msg_offset.Union(), builder, args);
    }
    else {
        ZstLog::net(LogLevel::debug, "Not connected to stage. Skipping to cleanup. {}");
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
	ZstLog::net(LogLevel::debug, "Timer cancel status: {}", ec.message());
	if (m_client_transport)
		m_client_transport->disconnect();

	if (m_tcp_graph_transport)
		m_tcp_graph_transport->disconnect();

#ifdef ZST_BUILD_DRAFT_API
	if (m_udp_graph_transport)
		m_udp_graph_transport->disconnect();
#endif

    //Mark all locally registered entites as deactivated
    ZstEntityBundle bundle;
    m_session->hierarchy()->get_local_performer()->get_factories(bundle);
    m_session->hierarchy()->get_local_performer()->get_child_entities(bundle, true);
    for (auto c : bundle) {
		synchronisable_enqueue_deactivation(c);
    }

	//Remove entities immediately
    if(sendtype == ZstTransportRequestBehaviour::SYNC_REPLY)
	    process_events();

    //Enqueue event for adaptors
    m_session->dispatch_disconnected_from_stage();
    ZstEventDispatcher<std::shared_ptr<ZstConnectionAdaptor>>::defer([this](std::shared_ptr<ZstConnectionAdaptor> adaptor) {
        if(m_api)
            adaptor->on_disconnected_from_stage(this->m_api, this->m_connected_server);
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
    ZstEventDispatcher<std::shared_ptr<ZstConnectionAdaptor> >::defer([this, server](std::shared_ptr<ZstConnectionAdaptor> adaptor) {
        adaptor->on_server_lost(m_api, server);
    });
}

void ZstClient::auto_join_stage_complete()
{
}

void ZstClient::start_connection_broadcast_handler(const std::shared_ptr<ZstStageMessage>& msg)
{
    auto request = msg->buffer()->content_as_ClientGraphHandshakeStart();
    auto path = request->receiver_URI();
    if (!path) {
        ZstLog::net(LogLevel::warn, "Received malformed ClientGraphHandshakeStart message");
        return;
    }
    auto remote_client_path = ZstURI(path->c_str(), path->size());
    
    ZstPerformer* local_client = session()->hierarchy()->get_local_performer();
    ZstPerformer* remote_client = dynamic_cast<ZstPerformer*>(session()->hierarchy()->find_entity(remote_client_path));
    ZstLog::net(LogLevel::debug, "Starting peer handshake broadcast to {}", remote_client->URI().path());

    if (!remote_client) {
        ZstLog::net(LogLevel::error, "Could not find performer {}", remote_client_path.path());
        return;
    }

    boost::asio::deadline_timer timer(m_client_timerloop.IO_context(), boost::posix_time::milliseconds(100));
    m_connection_timers->insert({ remote_client->URI(),  std::move(timer) });
    m_connection_timers->at(remote_client->URI()).async_wait(boost::bind(
        &ZstClient::send_connection_broadcast,
        &m_connection_timers->at(remote_client->URI()),
        this,
        remote_client->URI(),
        local_client->URI(),
        boost::posix_time::milliseconds(100)
    ));
}

void ZstClient::send_connection_broadcast(boost::asio::deadline_timer* t, ZstClient* client, const ZstURI& to, const ZstURI& from, boost::posix_time::milliseconds duration)
{
    ZstLog::net(LogLevel::debug, "Sending connection handshake. From: {}, To: {}", from.path(), to.path());

    ZstTransportArgs args;
    args.msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;

    auto builder = std::make_shared<FlatBufferBuilder>();
    auto plugval_offset = CreatePlugValue(*builder, PlugValueData_PlugHandshake, CreatePlugHandshake(*builder).Union());
    auto conn_msg = CreateGraphMessage(*builder, builder->CreateString(from.path(), from.full_size()), plugval_offset);
    client->m_tcp_graph_transport->send_msg(conn_msg, builder, args);
    
    if (client->m_connection_timers->find(to) != client->m_connection_timers->end()) {
        //Loop timer if it is valid
        t->expires_at(t->expires_at() + duration);
        t->async_wait(boost::bind(&ZstClient::send_connection_broadcast, t, client, to, from, duration));
    }
}

void ZstClient::stop_connection_broadcast_handler(const std::shared_ptr<ZstStageMessage>& msg)
{
    auto request = msg->buffer()->content_as_ClientGraphHandshakeStop();
    auto remote_client_path = ZstURI(request->receiver_URI()->c_str());
    ZstPerformer* remote_client = dynamic_cast<ZstPerformer*>(session()->hierarchy()->find_entity(remote_client_path));
    ZstLog::net(LogLevel::debug, "Stopping peer handshake broadcast to {}", remote_client->URI().path());

    if (!remote_client) {
        ZstLog::net(LogLevel::error, "Could not find performer {}", remote_client_path.path());
        return;
    }

    if (m_connection_timers->find(remote_client->URI()) != m_connection_timers->end()) {
        m_connection_timers->at(remote_client->URI()).cancel();
        m_connection_timers->at(remote_client->URI()).wait();
        m_connection_timers->erase(remote_client->URI());
    }
}

void ZstClient::listen_to_client_handler(const std::shared_ptr<ZstStageMessage>& msg)
{
    auto request = msg->buffer()->content_as_ClientGraphHandshakeListen();
    auto output_path = ZstURI(request->sender_URI()->c_str(), request->sender_URI()->size());
    
    ZstLog::net(LogLevel::debug, "Listening to performer {}", request->sender_address()->str());
    m_pending_peer_connections[output_path] = msg->id();
    m_tcp_graph_transport->connect(request->sender_address()->str());
}

std::shared_ptr<ZstClientSession> ZstClient::session()
{
    return m_session;
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
    entity->get_child_entities(bundle, true, true);

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
// -------------------


