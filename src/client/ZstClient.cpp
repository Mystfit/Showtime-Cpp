#include <chrono>
#include <sstream>

#include "ZstClient.h"

ZstClient::ZstClient() :
	m_heartbeat_timer_id(-1),
    m_ping(-1),
    m_is_ending(false),
    m_is_destroyed(false),
    m_init_completed(false),
    m_connected_to_stage(false),
    m_session(NULL)
{
	//Message and transport modules
	//These are specified by the client based on what transport type we want to use
	m_actor = new ZstActor();
	m_client_transport = new ZstClientTransport();
	m_graph_transport = new ZstGraphTransport();
	m_session = new ZstClientSession();
}

ZstClient::~ZstClient() {
	delete m_session;
	delete m_client_transport;
	delete m_graph_transport;
	delete m_actor;
}

ZstClient & ZstClient::instance()
{
	static ZstClient client_singleton;
	return client_singleton;
}

void ZstClient::destroy() {
	//Only need to call cleanup once
	if (m_is_ending || m_is_destroyed)
		return;
    m_is_ending = true;
    m_init_completed = false;
    
    //Let stage know we are leaving
	if(is_connected_to_stage())
		leave_stage();

	this->remove_adaptor(m_client_transport);

	m_actor->stop_loop();
	m_session->destroy();
	m_client_transport->destroy();
	m_graph_transport->destroy();
	m_actor->destroy();

	m_is_ending = false;
	m_is_destroyed = true;
}

void ZstClient::init_client(const char *client_name, bool debug)
{
	if (m_is_ending || m_init_completed) {
		ZstLog::net(LogLevel::notification, "Showtime already initialised");
		return;
	}

	m_is_destroyed = false;

	LogLevel level = LogLevel::notification;
	if (debug)
		level = LogLevel::debug;

	ZstLog::init_logger(client_name, level);
	ZstLog::net(LogLevel::notification, "Starting Showtime v{}", SHOWTIME_VERSION);

	m_client_name = client_name;

	//Todo: init IDs again after stage has responded
	ZstMsgIDManager::init(m_client_name.c_str(), m_client_name.size());

	m_actor->init();
	
	//Register message dispatch as a client adaptor
	this->add_adaptor(m_client_transport);

	//Register adaptors to handle outgoing events
	m_session->init(client_name);
	m_session->performance_events().add_adaptor(m_graph_transport);
	m_session->stage_events().add_adaptor(m_client_transport);
	m_session->hierarchy()->stage_events().add_adaptor(m_client_transport);

	//Set up client->stage adaptors
	m_client_transport->init(m_actor);
	m_client_transport->msg_events()->add_adaptor(this);
	m_client_transport->msg_events()->add_adaptor(m_session);
	m_client_transport->msg_events()->add_adaptor(m_session->hierarchy());

	//Set up client->performance adaptors
	m_graph_transport->init(m_actor);
	m_graph_transport->msg_events()->add_adaptor(this);
	m_graph_transport->msg_events()->add_adaptor(m_session);

	//Start the reactor
	m_actor->start_loop();

	//Init completed
    m_init_completed = true;
}

void ZstClient::init_file_logging(const char * log_file_path)
{
	ZstLog::init_file_logging(log_file_path);
}

void ZstClient::process_events()
{
    if(!is_init_complete() || m_is_destroyed){
        return;
    }

	m_client_transport->process_events();
	m_graph_transport->process_events();
	m_session->process_events();
}

void ZstClient::flush()
{
    ZstEventDispatcher<ZstTransportAdaptor*>::flush();
	m_session->flush();
}

// -----------------------
// Stage adaptor overrides
// -----------------------

void ZstClient::on_receive_msg(ZstMessage * msg)
{	
	switch (msg->kind()) {
	case ZstMsgKind::CONNECTION_HANDSHAKE:
		receive_connection_handshake(msg);
		break;
	case ZstMsgKind::START_CONNECTION_HANDSHAKE:
	{
		start_connection_broadcast(ZstURI(msg->get_arg("inputpath")));
		break;
	}
	case ZstMsgKind::STOP_CONNECTION_HANDSHAKE:
	{
		stop_connection_broadcast(ZstURI(msg->get_arg("inputpath")));
		break;
	}
	case ZstMsgKind::SUBSCRIBE_TO_PERFORMER:
	{
		//Listen for incoming handshake performance messages
		m_pending_peer_connections[ZstURI(msg->get_arg("outputpath"))] = std::stoull(msg->get_arg("connID"));
		
		//Start connecting to other peer
		m_graph_transport->connect_to_client(msg->get_arg("outputaddress"));
		break;
	}
	default:
		break;
	}
}


// ------------------------------
// Performance dispatch overrides
// ------------------------------

void ZstClient::receive_connection_handshake(ZstMessage * msg)
{
	//Peer connection is successful if we receive a handshake performance message from the sending client
	ZstURI output_path(msg->get_arg("outputpath"));
	if(m_pending_peer_connections.find(output_path) != m_pending_peer_connections.end()){
		invoke([this, &output_path](ZstTransportAdaptor* adaptor) {
			std::stringstream ss;
			ss << m_pending_peer_connections[output_path];
			ZstMsgArgs args{ 
				{"publisher", output_path.path()},
				{"connID", ss.str()}
			};
			adaptor->send_message(ZstMsgKind::SUBSCRIBE_TO_PERFORMER_ACK, ZstTransportSendType::ASYNC_REPLY, args, [](ZstMessageReceipt receipt) {
				if(receipt.status != ZstMsgKind::OK){
					ZstLog::net(LogLevel::error, "SUBSCRIBE_TO_PERFORMER_ACK: Stage responded with error {}", receipt.status);
				}
			});
		});
		
		m_pending_peer_connections.erase(output_path);
	}
}


// -------------
// Client join
// -------------

void ZstClient::join_stage(std::string stage_address, const ZstTransportSendType & sendtype) {
	
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
	m_is_connecting = true;
	
	ZstLog::net(LogLevel::notification, "Connecting to stage {}", stage_address);
	m_client_transport->connect_to_stage(stage_address);

	//Acquire our output graph address to send to the stage
	std::string graph_addr = m_graph_transport->get_graph_address();
	ZstPerformer * root = session()->hierarchy()->get_local_performer();

	invoke([this, sendtype, &graph_addr, root](ZstTransportAdaptor * adaptor) {
		adaptor->send_message(ZstMsgKind::CLIENT_JOIN, sendtype, *root, {{"graphaddress", graph_addr}}, [this](ZstMessageReceipt response) {
			this->join_stage_complete(response);
		});
	});
}

void ZstClient::join_stage_complete(ZstMessageReceipt response)
{
	m_is_connecting = false;
	
	//If we didn't receive a OK signal, something went wrong
	if (response.status != ZstMsgKind::OK) {
        ZstLog::net(LogLevel::error, "Stage connection failed with with status: {}", ZstMsgNames[response.status]);
		leave_stage_complete();
        return;
	}

	ZstLog::net(LogLevel::notification, "Connection to server established");

	//Set up heartbeat timer
	m_heartbeat_timer_id = m_actor->attach_timer(HEARTBEAT_DURATION, [this]() {this->heartbeat_timer(); });
	
	//TODO: Need a handshake with the stage before we mark connection as active
	m_connected_to_stage = true;

	//Ask the stage to send us the current session
	synchronise_graph(response.sendtype);

	//Enqueue connection events
	m_session->dispatch_connected_to_stage();

	//If we are sync, we can dispatch events immediately
	if (response.sendtype == ZstTransportSendType::SYNC_REPLY)
		m_session->process_events();
}

void ZstClient::synchronise_graph(const ZstTransportSendType & sendtype)
{
	//Ask the stage to send us a full snapshot
	ZstLog::net(LogLevel::notification, "Requesting stage snapshot");

	invoke([this, sendtype](ZstTransportAdaptor * adaptor) {
		adaptor->send_message(ZstMsgKind::CLIENT_SYNC, sendtype, [this](ZstMessageReceipt response) {
			this->synchronise_graph_complete(response);
		});
	});
}

void ZstClient::synchronise_graph_complete(ZstMessageReceipt response)
{
	ZstLog::net(LogLevel::notification, "Graph sync completed");
}

void ZstClient::leave_stage()
{
	if (m_connected_to_stage) {
		ZstLog::net(LogLevel::notification, "Leaving stage");

		//Set flags early to avoid double leaving shenanigans
		m_is_connecting = false;
		m_connected_to_stage = false;
        
		invoke([this](ZstTransportAdaptor * adaptor) { adaptor->send_message(ZstMsgKind::CLIENT_LEAVING); });
    } else {
        ZstLog::net(LogLevel::debug, "Not connected to stage. Skipping to cleanup. {}");
    }

	this->leave_stage_complete();
	this->process_events();
}

void ZstClient::leave_stage_complete()
{   
	//Set stage as disconnected again - just to make sure
	m_connected_to_stage = false;

	//Disconnect rest of sockets and timers
	if (m_heartbeat_timer_id > 0) {
		m_actor->detach_timer(m_heartbeat_timer_id);
		m_heartbeat_timer_id = -1;
	}
	m_client_transport->disconnect_from_stage();

	//Enqueue event for adaptors
	m_session->dispatch_disconnected_from_stage();
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

void ZstClient::heartbeat_timer(){
	invoke([this](ZstTransportAdaptor * adaptor) {
		std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
		adaptor->send_message(ZstMsgKind::CLIENT_HEARTBEAT, ZstTransportSendType::ASYNC_REPLY, [this, start](ZstMessageReceipt response) {
			if (response.status != ZstMsgKind::OK) {
				ZstLog::net(LogLevel::warn, "Server ping timed out");
				this->leave_stage_complete();
				return;
			}
			std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
			auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
			this->m_ping = static_cast<long>(delta.count());
		});
	});
}

void ZstClient::start_connection_broadcast(const ZstURI & remote_client_path)
{
	ZstPerformer * local_client = session()->hierarchy()->get_local_performer();
	ZstPerformer * remote_client = dynamic_cast<ZstPerformer*>(session()->hierarchy()->find_entity(remote_client_path));
	if(!remote_client){
		ZstLog::net(LogLevel::error, "Could not find performer {}", remote_client_path.path());
		return;
	}

	m_connection_timers[remote_client->URI()] = m_actor->attach_timer(100, [this, local_client](){
		//Cheat by sending a performance message with only the sender and no payload
		m_graph_transport->send_message(ZstMsgKind::CONNECTION_HANDSHAKE, { {"outputpath", local_client->URI().path()} });
	});
}

void ZstClient::stop_connection_broadcast(const ZstURI & remote_client_path)
{
	ZstPerformer * remote_client = dynamic_cast<ZstPerformer*>(session()->hierarchy()->find_entity(remote_client_path));
	if (!remote_client) {
		ZstLog::net(LogLevel::error, "Could not find performer {}", remote_client_path.path());
		return;
	}

	int timer_id = -1;
	try{
		timer_id = m_connection_timers[remote_client->URI()];
	} catch (std::out_of_range){
		ZstLog::net(LogLevel::warn, "Broadcast timer for {} not found", remote_client->URI().path());
	}

	if(timer_id > -1){
		m_actor->detach_timer(timer_id);
	}
}

ZstClientSession * ZstClient::session()
{
	return m_session;
}

