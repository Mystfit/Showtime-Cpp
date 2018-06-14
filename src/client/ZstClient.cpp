#include <chrono>
#include <sstream>

#include "ZstClient.h"

ZstClient::ZstClient() :
    m_ping(-1),
    m_is_ending(false),
    m_is_destroyed(false),
    m_init_completed(false),
    m_connected_to_stage(false),
	m_session(NULL)
{
	//Message and transport modules
	//These are specified by the client based on what transport type we want to use
	m_transport = new ZstCZMQTransportLayer();
	m_msg_dispatch = new ZstMessageDispatcher();
	m_transport->set_dispatcher(m_msg_dispatch);
	m_msg_dispatch->set_transport(m_transport);

	//Register client as a stage sender/receiver
	this->add_adaptor(m_msg_dispatch);

	m_msg_dispatch->stage_events().add_adaptor(this);
	m_msg_dispatch->performance_events().add_adaptor(this);
}

ZstClient::~ZstClient() {
	destroy();	

	delete m_session;
	delete m_msg_dispatch;
	delete m_transport;
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
		leave_stage(true);
	
	m_session->destroy();
	m_msg_dispatch->destroy();
	m_transport->destroy();
	
	m_is_ending = false;
	m_is_destroyed = true;
}

void ZstClient::init_client(const char *client_name, bool debug)
{
	if (m_is_ending || m_init_completed) {
		return;
	}

	ZstLog::init_logger(client_name, LogLevel::debug);
	ZstLog::net(LogLevel::notification, "Starting Showtime v{}", SHOWTIME_VERSION);

	m_client_name = client_name;

	m_session = new ZstClientSession();
	m_session->init(client_name);

	//Register adaptors to handle outgoing events
	m_session->stage_events().add_adaptor(m_msg_dispatch);
	m_session->hierarchy()->stage_events().add_adaptor(m_msg_dispatch);
	m_session->performance_events().add_adaptor(m_msg_dispatch);

	//Register adaptors to handle incoming events
	m_msg_dispatch->stage_events().add_adaptor(this);
	m_msg_dispatch->stage_events().add_adaptor(m_session);
	m_msg_dispatch->stage_events().add_adaptor(m_session->hierarchy());
	m_msg_dispatch->performance_events().add_adaptor(m_session);
	
	m_is_destroyed = false;
	m_transport->init();
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

	m_msg_dispatch->process_events();
	m_session->process_events();
}

void ZstClient::flush()
{
	this->flush();
	m_session->flush();
}

// -----------------------
// Stage adaptor overrides
// -----------------------

void ZstClient::on_receive_from_stage(ZstStageMessage * msg)
{	
	switch (msg->kind()) {
	case ZstMsgKind::START_CONNECTION_HANDSHAKE:
	{
		start_connection_broadcast(ZstURI(msg->payload_at(0).data(), msg->payload_at(0).size()));
		break;
	}
	case ZstMsgKind::STOP_CONNECTION_HANDSHAKE:
	{
		stop_connection_broadcast(ZstURI(msg->payload_at(0).data(), msg->payload_at(0).size()));
		break;
	}
	case ZstMsgKind::SUBSCRIBE_TO_PERFORMER:
	{
		//Listen for incoming handshake performance messages
		m_msg_dispatch->performance_events().add_adaptor(this);
		m_pending_peer_connections[ZstURI(msg->payload_at(0).data(), msg->payload_at(0).size())] = std::string(msg->payload_at(2).data(), msg->payload_at(2).size());

		//Start connecting to other peer
		m_transport->connect_to_client(std::string((char*)msg->payload_at(1).data(), msg->payload_at(1).size()).c_str());
		break;
	}
	default:
		break;
	}
}


// ------------------------------
// Performance dispatch overrides
// ------------------------------

void ZstClient::on_receive_from_performance(ZstPerformanceMessage * msg){
	if(msg->num_payloads() > 0){
		ZstLog::net(LogLevel::debug, "ZstClient received performance message with a payload, ignoring.");
		return;
	}

	//Peer connection is successful if we receive a handshake performance message from the sending client
	if(m_pending_peer_connections.find(msg->sender()) != m_pending_peer_connections.end()){

		std::vector<std::string> args{ msg->sender().path(), m_pending_peer_connections[msg->sender()] };
		msg_dispatch()->send_message(ZstMsgKind::SUBSCRIBE_TO_PERFORMER_ACK, true, args, [](ZstMessageReceipt receipt){
			if(receipt.status != ZstMsgKind::OK){
				ZstLog::net(LogLevel::error, "SUBSCRIBE_TO_PERFORMER_ACK: Stage responded with error {}", receipt.status);
			}
		});
		m_pending_peer_connections.erase(msg->sender());
	}
}


// -------------
// Client join
// -------------

void ZstClient::join_stage(std::string stage_address, bool async) {
	
	if (m_is_connecting || m_connected_to_stage) {
		ZstLog::net(LogLevel::error, "Connection in progress or already connected, can't connect to stage.");
		return;
	}
	m_is_connecting = true;
	
	ZstLog::net(LogLevel::notification, "Connecting to stage {}", stage_address);
	m_transport->connect_to_stage(stage_address);

	//Acquire our output graph address to send to the stage
	const char * graph_addr = m_transport->get_graph_address();

	ZstPerformer * root = session()->hierarchy()->get_local_performer();
	invoke([this, async, graph_addr, root](ZstStageDispatchAdaptor * adaptor) {
		adaptor->send_serialisable_message(ZstMsgKind::CLIENT_JOIN, *root, async, graph_addr, [this](ZstMessageReceipt response) {
			this->join_stage_complete(response);
		});
	});
}

void ZstClient::join_stage_complete(ZstMessageReceipt response)
{
	m_is_connecting = false;

	//If we didn't receive a OK signal, something went wrong
	if (response.status != ZstMsgKind::OK) {
        ZstLog::net(LogLevel::error, "Stage connection failed with with status: {}", response.status);
		leave_stage_complete();
        return;
	}

	ZstLog::net(LogLevel::notification, "Connection to server established");

	//Set up heartbeat timer
	m_heartbeat_timer_id = m_transport->add_timer(HEARTBEAT_DURATION, [this]() {this->heartbeat_timer(); });
	
	//TODO: Need a handshake with the stage before we mark connection as active
	m_connected_to_stage = true;

	//Ask the stage to send us the current session
	synchronise_graph(response.async);

	//Enqueue connection events
	m_session->dispatch_connected_to_stage();

	//If we are sync, we can dispatch events immediately
	if (!response.async)
		m_session->process_events();
}

void ZstClient::synchronise_graph(bool async)
{
	//Ask the stage to send us a full snapshot
	ZstLog::net(LogLevel::notification, "Requesting stage snapshot");

	invoke([this, async](ZstStageDispatchAdaptor * adaptor) {
		adaptor->send_message(ZstMsgKind::CLIENT_SYNC, async, [this](ZstMessageReceipt response) {
			this->synchronise_graph_complete(response);
		});
	});
}

void ZstClient::synchronise_graph_complete(ZstMessageReceipt response)
{
	ZstLog::net(LogLevel::notification, "Graph sync completed");
}

void ZstClient::leave_stage(bool async)
{
	if (m_connected_to_stage) {
		ZstLog::net(LogLevel::notification, "Leaving stage");
        
		invoke([this, async](ZstStageDispatchAdaptor * adaptor) {
			adaptor->send_message(ZstMsgKind::CLIENT_LEAVING, async, [this](ZstMessageReceipt response) {
				this->leave_stage_complete();
			});
		});
		
		//Run callbacks if we're still on the main app thread
		if(!async) process_events();
    } else {
        ZstLog::net(LogLevel::debug, "Not connected to stage. Skipping to cleanup. {}");
        leave_stage_complete();
		if (!async) process_events();
    }
}

void ZstClient::leave_stage_complete()
{   
    //Disconnect rest of sockets and timers
	m_transport->remove_timer(m_heartbeat_timer_id);
	m_transport->disconnect_from_stage();

	m_is_connecting = false;
	m_connected_to_stage = false;

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
	std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();

	invoke([this, &start](ZstStageDispatchAdaptor * adaptor) {
		adaptor->send_message(ZstMsgKind::CLIENT_HEARTBEAT, true, [this, &start](ZstMessageReceipt response) {
			std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
			std::chrono::milliseconds delta = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
			ZstLog::net(LogLevel::notification, "Ping roundtrip {} ms", delta.count());
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

	m_connection_timers[remote_client->URI()] = m_transport->add_timer(100, [this, local_client](){
		m_transport->send_to_performance( m_transport->get_performance_msg()->init_performance_message(local_client->URI()));
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
		m_transport->remove_timer(timer_id);
	}
}

ZstMessageDispatcher * ZstClient::msg_dispatch()
{
	return m_msg_dispatch;
}

ZstClientSession * ZstClient::session()
{
	return m_session;
}

