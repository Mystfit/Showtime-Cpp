#include <sstream>
#include <chrono>

//Core headers
#include <ZstVersion.h>

//Stage headers
#include "ZstPerformerStageProxy.h"
#include "ZstStage.h"

using namespace std;

ZstStage::ZstStage() : 
	m_is_destroyed(false),
	m_session(NULL),
	m_transport(NULL),
	m_dispatch(NULL)
{
	m_session = new ZstStageSession();
	m_transport = new ZstStageTransport();
	m_dispatch = new ZstTransportDispatcher();

	m_transport->set_dispatcher(m_dispatch);
	m_dispatch->set_transport(m_transport);

	m_stage_events.add_adaptor(m_dispatch);
}

ZstStage::~ZstStage()
{
	destroy();
	delete m_session;
	delete m_transport;
	delete m_dispatch;
}

void ZstStage::init(const char * stage_name)
{
	ZstLog::init_logger(stage_name, LogLevel::debug);
	ZstLog::net(LogLevel::notification, "Starting Showtime v{} stage server", SHOWTIME_VERSION);

	m_heartbeat_timer_id = attach_timer(stage_heartbeat_timer_func, HEARTBEAT_DURATION, this);
}

void ZstStage::destroy()
{
	if (is_destroyed())
		return;

	m_is_destroyed = true;

	m_stage_events.remove_all_adaptors();
	this->flush_events();

	m_session->destroy();
	m_transport->destroy();
	m_dispatch->destroy();
	
	detach_timer(m_heartbeat_timer_id);	
}

bool ZstStage::is_destroyed()
{
	return m_is_destroyed;
}

void ZstStage::process_events()
{
	m_stage_events.process_events();
}

void ZstStage::flush_events()
{
	m_stage_events.flush();
}

void ZstStage::on_receive_msg(ZstStageMessage * msg)
{
	ZstStageMessage * response = NULL;
	ZstPerformerStageProxy * sender = dynamic_cast<ZstPerformerStageProxy*>(m_session->hierarchy()->find_entity(msg->sender()));
	
	//Check client hasn't finished joining yet
	if (!sender)
		return;

	switch (msg->kind()) {
	case ZstMsgKind::CLIENT_SYNC:
		response = synchronise_client_graph(sender);
		break;
	case ZstMsgKind::CLIENT_HEARTBEAT:
	{
		sender->set_heartbeat_active();
		response = m_transport->msg_pool()->get_msg()->init_message(ZstMsgKind::OK);
		break;
	}
	default:
		break;
	}
	
	//Send ack
	if (response) {
		//Copy ID of the original message so we can match this message to a promise on the client
		//once upon a time there was a null pointer, it pointed to nothing.
		response->copy_id(msg);
		m_dispatch->send_to_address(response, sender);
	}

	return;
}



//---------------------
// Outgoing event queue
//---------------------


ZstStageMessage * ZstStage::synchronise_client_graph(ZstPerformer * client) {

	ZstLog::net(LogLevel::notification, "Sending graph snapshot to {}", client->URI().path());

	//Pack performer root entities
	for (auto performer : m_clients) {
		//Only pack performers that aren't the destination client
		if (performer.second->URI() != client->URI()) {
			send_to_client(m_transport->msg_pool()->get_msg()->init_entity_message(performer.second), client);
		}
	}
	
	//Pack cables
	for (auto cable : m_cables) {
		send_to_client(msg_pool()->get_msg()->init_serialisable_message(ZstMsgKind::CREATE_CABLE, *cable), client);
	}
	
    return msg_pool()->get_msg()->init_message(ZstMsgKind::OK);
}


// -------

int ZstStage::stage_heartbeat_timer_func(zloop_t * loop, int timer_id, void * arg)
{
	ZstStage * stage = (ZstStage*)arg;
	ZstClientMap clients = stage->m_clients;
	std::vector<ZstPerformer*> removed_clients;
	for (auto performer_it : stage->m_clients) {
		ZstPerformer * performer = performer_it.second;
		if (performer->get_active_heartbeat()) {
			performer->clear_active_hearbeat();
		}
		else {
			ZstLog::net(LogLevel::warn, "Client {} missed a heartbeat. {} remaining", performer->URI().path(), MAX_MISSED_HEARTBEATS - performer->get_missed_heartbeats());
			performer->set_heartbeat_inactive();
		}

		if (performer->get_missed_heartbeats() > MAX_MISSED_HEARTBEATS) {
			removed_clients.push_back(performer);
		}
	}

	for (auto client : removed_clients) {
		stage->destroy_client(client);
	}
	return 0;
}