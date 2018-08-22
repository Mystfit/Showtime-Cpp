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
	m_session(NULL),
	m_publisher_transport(NULL),
	m_router_transport(NULL),
	m_stage_eventloop(this),
	m_heartbeat_timer(m_stage_eventloop.IO_context())
{
	m_session = new ZstStageSession();
	m_publisher_transport = new ZstStagePublisherTransport();
	m_router_transport = new ZstStageRouterTransport();
}

ZstStage::~ZstStage()
{
delete m_session;
delete m_publisher_transport;
delete m_router_transport;
}

void ZstStage::init_stage(const char * stage_name, bool threaded)
{
	ZstLog::init_logger(stage_name, LogLevel::debug);
	ZstLog::net(LogLevel::notification, "Starting Showtime v{} stage server", SHOWTIME_VERSION);

	m_session->init();
	m_publisher_transport->init();
	m_router_transport->init();

	//Init timer actor for client heartbeats
	//Create timers
	m_heartbeat_timer.expires_from_now(boost::posix_time::milliseconds(STAGE_HEARTBEAT_CHECK));
	m_heartbeat_timer.async_wait(boost::bind(&ZstStage::stage_heartbeat_timer, &m_heartbeat_timer, this, boost::posix_time::milliseconds(STAGE_HEARTBEAT_CHECK)));

	//Attach adaptors
	m_router_transport->msg_events()->add_adaptor(m_session);
	m_router_transport->msg_events()->add_adaptor(m_session->hierarchy());
	m_session->router_events().add_adaptor(m_router_transport);
	m_session->publisher_events().add_adaptor(m_publisher_transport);
	m_session->hierarchy()->router_events().add_adaptor(m_router_transport);
	m_session->hierarchy()->publisher_events().add_adaptor(m_publisher_transport);

	//Start event loop
	m_stage_eventloop_thread = boost::thread(boost::ref(m_stage_eventloop));
}


void ZstStage::destroy()
{
	if (is_destroyed())
		return;

	m_is_destroyed = true;

	this->remove_all_adaptors();
	this->flush();

	//Remove timers
	m_heartbeat_timer.cancel();
	m_heartbeat_timer.wait();

	m_stage_eventloop_thread.interrupt();
	m_stage_eventloop_thread.join();

	m_session->destroy();
	m_publisher_transport->destroy();
	m_router_transport->destroy();

	//Destroy zmq context
	zsys_shutdown();
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

void ZstStage::stage_heartbeat_timer(boost::asio::deadline_timer * t, ZstStage * stage, boost::posix_time::milliseconds duration)
{
	std::vector<ZstPerformer*> removed_clients;
	for (auto performer : stage->m_session->hierarchy()->get_performers()) {
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
		stage->m_session->hierarchy()->remove_proxy_entity(client);
	}

	if (!stage->is_destroyed()){
		//Loop timer
		t->expires_at(t->expires_at() + duration);
		t->async_wait(boost::bind(&ZstStage::stage_heartbeat_timer, t, stage, duration));
	}
}



// ------

ZstStageIOLoop::ZstStageIOLoop(ZstStage * stage)
{
	m_stage = stage;
}

void ZstStageIOLoop::operator()()
{
	while (1) {
		try {
			boost::this_thread::interruption_point();
			m_stage->process_events();
			IO_context().poll();
		}
		catch (boost::thread_interrupted) {
			ZstLog::net(LogLevel::debug, "Stage event loop exiting.");
			break;
		}
	}
}

boost::asio::io_service & ZstStageIOLoop::IO_context()
{
	return m_io;
}
