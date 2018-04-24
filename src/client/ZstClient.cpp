#include <chrono>
#include <sstream>

#include "ZstClient.h"

ZstClient::ZstClient() :
    m_ping(-1),
    m_is_ending(false),
    m_is_destroyed(false),
    m_init_completed(false),
    m_connected_to_stage(false)
{
	//Client modules
	m_transport = new ZstCZMQTransportLayer(this);
	m_msg_dispatch = new ZstMessageDispatcher(this, m_transport);
	m_session = new ZstSession(this);
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

	ZstLog::init_logger(client_name);
	ZstLog::net(LogLevel::notification, "Starting Showtime v{}", SHOWTIME_VERSION);

	m_client_name = client_name;
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

	ZstEventDispatcher<ZstClientAdaptor*>::process_events();
	
	//Only delete entities after all adaptors have been processed
}

void ZstClient::flush()
{
	this->flush();
	m_session->flush();
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
    
	ZstStageMessage * msg = m_msg_dispatch->init_serialisable_message(ZstMsgKind::CLIENT_JOIN, *m_hierarchy->get_local_performer());
	msg->append_str(stage_address.c_str(), stage_address.size());
	m_msg_dispatch->send_to_stage(msg, async, [this](ZstMessageReceipt response) { this->join_stage_complete(response); });
	
	//Run callbacks if we're still on the main app thread
	if (!async) process_events();
}

void ZstClient::join_stage_complete(ZstMessageReceipt response)
{
	m_is_connecting = false;

	//If we didn't receive a OK signal, something went wrong
	if (response.status != ZstMsgKind::OK) {
        ZstLog::net(LogLevel::error, "Stage connection failed with with status: {}", response.status);
        return;
	}

	ZstLog::net(LogLevel::notification, "Connection to server established");

	//Set up heartbeat timer
	m_heartbeat_timer_id = m_transport->add_timer(HEARTBEAT_DURATION, [this]() {this->heartbeat_timer(); });
	
	//TODO: Need a handshake with the stage before we mark connection as active
	m_connected_to_stage = true;

	//Enqueue connection events
	add_event([](ZstClientAdaptor* adaptor) {adaptor->connected_to_stage(); });

	m_hierarchy->synchronise_graph(response.async);
}

void ZstClient::leave_stage(bool async)
{
	if (m_connected_to_stage) {
		ZstLog::net(LogLevel::notification, "Leaving stage");
        
        ZstStageMessage * msg = m_msg_dispatch->init_message(ZstMsgKind::CLIENT_LEAVING);
		m_msg_dispatch->send_to_stage(msg, async, [this](ZstMessageReceipt response) {this->leave_stage_complete(); });

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
    //Deactivate all owned entities
    m_hierarchy->get_local_performer()->enqueue_deactivation();
    
    //Disconnect rest of sockets and timers
	m_transport->remove_timer(m_heartbeat_timer_id);
	m_transport->disconnect_from_stage();

	m_is_connecting = false;
	m_connected_to_stage = false;

	//Enqueue event for adaptors
	add_event([](ZstClientAdaptor* adaptor) { adaptor->disconnected_from_stage(); });
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

	ZstStageMessage * msg = m_msg_dispatch->init_message(ZstMsgKind::CLIENT_HEARTBEAT);
	m_msg_dispatch->send_to_stage(msg, true, [&start, this](ZstMessageReceipt response) {
		std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
		std::chrono::milliseconds delta = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		ZstLog::net(LogLevel::notification, "Ping roundtrip {} ms", delta.count());
		this->m_ping = static_cast<long>(delta.count());
	});
}

ZstMessageDispatcher * ZstClient::msg_dispatch()
{
	return m_msg_dispatch;
}
