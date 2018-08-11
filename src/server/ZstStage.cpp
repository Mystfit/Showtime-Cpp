#include <sstream>
#include <chrono>
#include <boost/lexical_cast.hpp>

#include "ZstStage.h"

//Core headers
#include <ZstVersion.h>

//Stage headers
#include "ZstPerformerStageProxy.h"
#include "ZstStage.h"

using namespace std;

ZstStage::ZstStage() : 
	m_is_destroyed(false),
	m_actor(NULL),
	m_session(NULL),
	m_publisher_transport(NULL),
	m_router_transport(NULL)
{
	m_session = new ZstStageSession();
	m_publisher_transport = new ZstStagePublisherTransport();
	m_router_transport = new ZstStageRouterTransport();
	m_actor = new ZstActor();
}

ZstStage::~ZstStage()
{
	delete m_session;
	delete m_publisher_transport;
	delete m_router_transport;
	delete m_actor;;
}

void ZstStage::init_stage(const char * stage_name, bool threaded)
{
	ZstLog::init_logger(stage_name, LogLevel::debug);
	ZstLog::net(LogLevel::notification, "Starting Showtime v{} stage server", SHOWTIME_VERSION);
	
	m_actor->init(stage_name);
	m_session->init();
	m_publisher_transport->init(m_actor);
	m_router_transport->init(m_actor);

	m_heartbeat_timer_id = m_actor->attach_timer(HEARTBEAT_DURATION, [this]() {this->stage_heartbeat_timer_func(); });
	m_actor->start_loop();

	//Attach adaptors
	m_router_transport->msg_events()->add_adaptor(m_session);
	m_router_transport->msg_events()->add_adaptor(m_session->hierarchy());
	m_session->router_events().add_adaptor(m_router_transport);
	m_session->publisher_events().add_adaptor(m_publisher_transport);
	m_session->hierarchy()->router_events().add_adaptor(m_router_transport);
	m_session->hierarchy()->publisher_events().add_adaptor(m_publisher_transport);

	//Start event loop
	m_eventloop_thread = boost::thread(ZstStageLoop(this));
}


void ZstStage::destroy()
{
	if (is_destroyed())
		return;

	m_is_destroyed = true;

	this->remove_all_adaptors();
	this->flush();

	m_eventloop_thread.interrupt();
	m_eventloop_thread.join();
	m_actor->stop_loop();
	m_session->destroy();
	m_publisher_transport->destroy();
	m_router_transport->destroy();
	m_actor->detach_timer(m_heartbeat_timer_id);
	m_actor->destroy();
}

bool ZstStage::is_destroyed()
{
	return m_is_destroyed;
}

void ZstStage::process_events()
{
	m_session->process_events();
	m_publisher_transport->process_events();
	m_router_transport->process_events();

	//Reapers are updated last in case entities still need to be queried beforehand
	m_session->reaper().reap_all();
	m_session->hierarchy()->reaper().reap_all();
}


//---------------------
// Outgoing event queue
//---------------------

void ZstStage::stage_heartbeat_timer_func()
{
	std::vector<ZstPerformer*> removed_clients;
	for (auto performer : m_session->hierarchy()->get_performers()) {
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
		m_session->hierarchy()->remove_proxy_entity(client);
	}
}

ZstStageLoop::ZstStageLoop(ZstStage * stage)
{
	m_stage = stage;
}


// ------


void ZstStageLoop::operator()()
{
	//std::unique_lock< boost::fibers::mutex > lk(m_event_loop_lock);
	while (1) {
		try {
			//while (!data_ready) {
			//	cond.wait(lk);
			//}
			boost::this_thread::interruption_point();
			m_stage->process_events();
		}
		catch (boost::thread_interrupted) {
			ZstLog::net(LogLevel::debug, "Stage event loop exiting.");
			break;
		}
	}
}
