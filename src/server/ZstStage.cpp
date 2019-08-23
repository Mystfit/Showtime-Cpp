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
	m_heartbeat_timer(m_io.IO_context()),
	m_event_condition(std::make_shared<ZstSemaphore>()),
    m_session(std::make_unique<ZstStageSession>()),
    m_router_transport(std::make_unique<ZstZMQServerTransport>()),
    m_websocket_transport(std::make_shared<ZstWebsocketServerTransport>(m_io)),
    m_service_broadcast_transport(std::make_unique<ZstServiceDiscoveryTransport>())
{	
	//Register event conditions
	m_session->set_wake_condition(m_event_condition);
	m_router_transport->msg_events()->set_wake_condition(m_event_condition);
	m_websocket_transport->msg_events()->set_wake_condition(m_event_condition);
	this->set_wake_condition(m_event_condition);
}

ZstStage::~ZstStage()
{
	destroy();
}

void ZstStage::init_stage(const char * stage_name, int port)
{
	m_session->init();
	m_router_transport->init();
	m_router_transport->bind(fmt::format("*:{}", port));
	m_websocket_transport->init();
	m_websocket_transport->bind("127.0.0.1");
    
    //Stage discovery beacon
    m_service_broadcast_transport->init(STAGE_DISCOVERY_PORT);
    
    //We start the beacon broadcast by sending a message with the intended broadcast data
	ZstTransportArgs args;
	args.msg_args = {
		{get_msg_arg_name(ZstMsgArg::NAME), stage_name},
		{get_msg_arg_name(ZstMsgArg::ADDRESS_PORT), port}
	};
    m_service_broadcast_transport->send_msg(ZstMsgKind::SERVER_BEACON, args);

	//Init timer actor for client heartbeats
	//Create timers
	m_heartbeat_timer.expires_from_now(boost::posix_time::milliseconds(STAGE_HEARTBEAT_CHECK));
	m_heartbeat_timer.async_wait(boost::bind(&ZstStage::stage_heartbeat_timer, &m_heartbeat_timer, this, boost::posix_time::milliseconds(STAGE_HEARTBEAT_CHECK)));

	//Attach tcp transport adaptors
	m_router_transport->msg_events()->add_adaptor(static_cast<ZstTransportAdaptor*>(m_session.get()));
	m_router_transport->msg_events()->add_adaptor(static_cast<ZstTransportAdaptor*>(m_session->hierarchy()));
	m_session->router_events().add_adaptor(m_router_transport.get());
	m_session->hierarchy()->router_events().add_adaptor(m_router_transport.get());
	
	//Attach websocket transport adaptors
	m_websocket_transport->msg_events()->add_adaptor(static_cast<ZstTransportAdaptor*>(m_session.get()));
	m_websocket_transport->msg_events()->add_adaptor(static_cast<ZstTransportAdaptor*>(m_session->hierarchy()));
	m_session->router_events().add_adaptor(m_websocket_transport.get());
	m_session->hierarchy()->router_events().add_adaptor(m_websocket_transport.get());

	//Start event loop
	m_stage_timer_thread = boost::thread(boost::bind(&ZstStage::timer_loop, this));
	m_stage_eventloop_thread = boost::thread(boost::bind(&ZstStage::event_loop, this));
}


void ZstStage::destroy()
{
	if (is_destroyed())
		return;

	m_is_destroyed = true;

	//Remove timers
	m_heartbeat_timer.cancel();
	m_heartbeat_timer.wait();

	//Destroy transports
    m_service_broadcast_transport->stop_broadcast();
    
    //Stop threads
    m_stage_eventloop_thread.interrupt();
    m_event_condition->notify();
    m_stage_eventloop_thread.try_join_for(boost::chrono::milliseconds(250));
    m_stage_timer_thread.interrupt();
    m_io.IO_context().stop();
    m_stage_timer_thread.join();

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
	m_websocket_transport->process_events();
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
		boost::asio::io_context::work work(m_io.IO_context());

		//Run the event loop (blocks this thread)
		this->m_io.IO_context().run();
	}
	catch (boost::thread_interrupted) {
		ZstLog::server(LogLevel::debug, "Stage timer event loop exiting.");
	}
}
