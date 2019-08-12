#include <sstream>
#include <chrono>

#include "ZstStage.h"

//Core headers
#include <ZstVersion.h>
#include "../core/ZstSemaphore.h"

//Stage headers
#include "ZstPerformerStageProxy.h"

ZstStage::ZstStage() : 
	m_is_destroyed(false),
	m_heartbeat_timer(m_io),
    m_session(NULL)
{
	m_session = new ZstStageSession();
	m_router_transport = std::make_unique<ZstZMQServerTransport>();
    m_service_broadcast_transport = std::make_unique<ZstServiceDiscoveryTransport>();

	//Register event conditions
	m_event_condition = std::make_shared<ZstSemaphore>();
	m_session->set_wake_condition(m_event_condition);
	m_router_transport->msg_events()->set_wake_condition(m_event_condition);
	this->set_wake_condition(m_event_condition);
}

ZstStage::~ZstStage()
{
	destroy();
	delete m_session;
}

void ZstStage::init_stage(const char * stage_name, int port)
{
	m_session->init();
	m_router_transport->init(port);
    
    //Stage discovery beacon
    m_service_broadcast_transport->init(STAGE_DISCOVERY_PORT);
    
    //We start the beacon broadcast by sending a message with the intended broadcast data
    m_service_broadcast_transport->send_msg(ZstMsgKind::SERVER_BEACON, {
        {get_msg_arg_name(ZstMsgArg::SENDER), stage_name},
        {get_msg_arg_name(ZstMsgArg::ADDRESS_PORT), port}
    });

	//Init timer actor for client heartbeats
	//Create timers
	m_heartbeat_timer.expires_from_now(boost::posix_time::milliseconds(STAGE_HEARTBEAT_CHECK));
	m_heartbeat_timer.async_wait(boost::bind(&ZstStage::stage_heartbeat_timer, &m_heartbeat_timer, this, boost::posix_time::milliseconds(STAGE_HEARTBEAT_CHECK)));

	//Attach adaptors
	m_router_transport->msg_events()->add_adaptor(static_cast<ZstTransportAdaptor*>(m_session));
	m_router_transport->msg_events()->add_adaptor(static_cast<ZstTransportAdaptor*>(m_session->hierarchy()));
	m_session->router_events().add_adaptor(m_router_transport.get());
	m_session->hierarchy()->router_events().add_adaptor(m_router_transport.get());

	//Start event loop
	m_stage_timer_thread = boost::thread(boost::bind(&ZstStage::timer_loop, this));
	m_stage_eventloop_thread = boost::thread(boost::bind(&ZstStage::event_loop, this));
}


void ZstStage::destroy()
{
	if (is_destroyed())
		return;

	m_is_destroyed = true;

	//Clear adaptors and events
	this->remove_all_adaptors();
	this->flush();

	//Remove timers
	m_heartbeat_timer.cancel();
	m_heartbeat_timer.wait();
    
    //Destroy modules
    m_session->destroy();

	//Stop threads
	m_stage_eventloop_thread.interrupt();
	m_event_condition->notify();
    m_stage_eventloop_thread.try_join_for(boost::chrono::milliseconds(250));
	m_stage_timer_thread.interrupt();
	m_io.stop();
	m_stage_timer_thread.join();

	//Destroy transports
	m_router_transport->destroy();
    
    m_service_broadcast_transport->stop_broadcast();
    m_service_broadcast_transport->destroy();

	//Destroy zmq context
	//zsys_shutdown();
}

bool ZstStage::is_destroyed()
{
	return m_is_destroyed;
}

void ZstStage::process_events()
{
	m_session->process_events();
	m_router_transport->process_events();

	//Reapers are updated last in case entities still need to be queried beforehand
//    m_session->reaper().reap_all();
//    m_session->hierarchy()->reaper().reap_all();
}


//---------------------
// Outgoing event queue
//---------------------

void ZstStage::stage_heartbeat_timer(boost::asio::deadline_timer * t, ZstStage * stage, boost::posix_time::milliseconds duration)
{
	std::vector<ZstPerformer*> removed_clients;
	ZstEntityBundle bundle;
	for (auto entity : stage->m_session->hierarchy()->get_performers(bundle)) {
		ZstPerformer * performer = dynamic_cast<ZstPerformer*>(entity);
		if (performer->get_active_heartbeat()) {
			performer->clear_active_hearbeat();
		}
		else {
			ZstLog::server(LogLevel::warn, "Client {} missed a heartbeat. {} remaining", performer->URI().path(), MAX_MISSED_HEARTBEATS - performer->get_missed_heartbeats());
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

void ZstStage::event_loop() 
{
	while (1) {
		try {
			boost::this_thread::interruption_point();
			this->m_event_condition->wait();    
            if(this->is_destroyed())
                break;
            this->process_events();
		}
		catch (boost::thread_interrupted) {
			ZstLog::server(LogLevel::debug, "Stage msg event loop exiting.");
			break;
		}
	}
}

void ZstStage::timer_loop()
{
	try {
		boost::this_thread::interruption_point();

		//Give the event loop some work to do so it doesn't insta-quit
		boost::asio::io_context::work work(m_io);

		//Run the event loop (blocks this thread)
		this->m_io.run();
	}
	catch (boost::thread_interrupted) {
		ZstLog::server(LogLevel::debug, "Stage timer event loop exiting.");
	}
}
