#include <sstream>
#include <chrono>


//Core headers
#include "showtime/ZstVersion.h"
#include "../core/ZstSemaphore.h"

//Stage headers
#include "ZstPerformerStageProxy.h"
#include "ZstStage.h"

// Include czmq last to avoid conflicts with boost::beast
#include <czmq.h>


using namespace flatbuffers;

namespace showtime::detail
{
	ZstStage::ZstStage() :
		m_is_destroyed(false),
		m_heartbeat_timer(m_io.IO_context()),
		m_event_condition(std::make_shared<ZstSemaphore>()),
		m_session(std::make_shared<ZstStageSession>()),
		m_router_transport(std::make_shared<ZstZMQServerTransport>()),
		m_websocket_transport(std::make_shared<ZstWebsocketServerTransport>(m_io)),
		m_service_broadcast_transport(std::make_shared<ZstServiceDiscoveryTransport>())
	{
		//Register event conditions
		m_session->set_wake_condition(m_event_condition);
	}

	ZstStage::~ZstStage()
	{
		destroy();
	}

	void ZstStage::init(const char* server_name, int port, bool unlisted)
	{
		// Set up logging
		auto log_events = ZstEventDispatcher< std::shared_ptr<ZstLogAdaptor> >::downcasted_shared_from_this<ZstEventDispatcher< std::shared_ptr<ZstLogAdaptor> >>();
		Log::init_logger(server_name, Log::Level::debug, log_events);
		Log::server(Log::Level::notification, "Starting Showtime v{} server", SHOWTIME_VERSION_STRING);

		// Set up transports
		m_router_transport->init();
		m_router_transport->bind(fmt::format("*:{}", (port > 0) ? std::to_string(port) : "*"));
		m_websocket_transport->init();
		m_websocket_transport->bind("127.0.0.1");

		//Set up module adaptors
		m_session->init_adaptors();
		m_session->hierarchy()->init_adaptors();

		//Stage discovery beacon
		m_service_broadcast_transport->init(STAGE_DISCOVERY_PORT); 
		
		//Init timer actor for client heartbeats
		//Create timers
		m_heartbeat_timer.expires_from_now(boost::posix_time::milliseconds(STAGE_HEARTBEAT_CHECK));
		m_heartbeat_timer.async_wait(boost::bind(&ZstStage::stage_heartbeat_timer, &m_heartbeat_timer, this, boost::posix_time::milliseconds(STAGE_HEARTBEAT_CHECK)));

		//Attach tcp transport adaptors
		m_router_transport->msg_events()->add_adaptor(std::static_pointer_cast<ZstTransportAdaptor>(m_session));
		m_router_transport->msg_events()->add_adaptor(std::static_pointer_cast<ZstTransportAdaptor>(m_session->stage_hierarchy()));
		m_session->router_events()->add_adaptor(std::static_pointer_cast<ZstTransportAdaptor>(m_router_transport));
		m_session->stage_hierarchy()->router_events()->add_adaptor(std::static_pointer_cast<ZstTransportAdaptor>(m_router_transport));

		//Attach websocket transport adaptors
		m_websocket_transport->msg_events()->add_adaptor(std::static_pointer_cast<ZstTransportAdaptor>(m_session));
		m_websocket_transport->msg_events()->add_adaptor(std::static_pointer_cast<ZstTransportAdaptor>(m_session->stage_hierarchy()));
		m_session->router_events()->add_adaptor(m_websocket_transport);
		m_session->stage_hierarchy()->router_events()->add_adaptor(m_websocket_transport);

		//Start event loop
		m_stage_timer_thread = boost::thread(boost::bind(&ZstStage::timer_loop, this));
		m_stage_eventloop_thread = boost::thread(boost::bind(&ZstStage::event_loop, this));

		if (!unlisted)
			start_broadcasting(server_name);
	}


	void ZstStage::destroy()
	{
		if (is_destroyed())
			return;

		m_is_destroyed = true;

		//Let clients know the server is shutting down
		send_shutdown_signal();

		//Remove timers
		m_heartbeat_timer.cancel();
		m_heartbeat_timer.wait();

		//Destroy transports
		m_service_broadcast_transport->stop_broadcast();
		m_service_broadcast_transport->destroy();
		m_websocket_transport->destroy();
		m_router_transport->destroy();

		//Stop threads
		m_stage_eventloop_thread.interrupt();
		m_event_condition->notify();
		m_stage_eventloop_thread.try_join_for(boost::chrono::milliseconds(250));
		m_stage_timer_thread.interrupt();
		m_io.IO_context().stop();
		m_stage_timer_thread.join();
	}

	bool ZstStage::is_destroyed()
	{
		return m_is_destroyed;
	}

	void ZstStage::start_broadcasting(const char* stage_name)
	{
		//Broadcast the server details to the local network
		m_service_broadcast_transport->start_broadcast(stage_name, m_router_transport->port(), HEARTBEAT_DURATION);
	}

	void ZstStage::stop_broadcasting()
	{
		m_service_broadcast_transport->stop_broadcast();
	}

	void ZstStage::send_shutdown_signal() {
		auto builder = std::make_shared<FlatBufferBuilder>();
		m_session->stage_hierarchy()->broadcast(
			Content_ServerStatusMessage, 
			CreateServerStatusMessage(*builder, ServerStatus_QUIT).Union(), 
			builder, 
			ZstTransportArgs()
		);
	}

	int ZstStage::port()
	{
		return m_router_transport->port();
	}

	void ZstStage::process_events()
	{
		m_session->process_events();
		ZstEventDispatcher< std::shared_ptr<ZstLogAdaptor> >::process_events();
	}


	//---------------------
	// Outgoing event queue
	//---------------------

	void ZstStage::stage_heartbeat_timer(boost::asio::deadline_timer* t, ZstStage* stage, boost::posix_time::milliseconds duration)
	{
		std::vector<ZstPerformer*> removed_clients;
		ZstEntityBundle bundle;
		for (auto entity : stage->m_session->hierarchy()->get_performers(bundle)) {
			ZstPerformer* performer = dynamic_cast<ZstPerformer*>(entity);
			if (performer){
				if (performer->get_active_heartbeat()) {
					performer->clear_active_hearbeat();
				}
				else {
					Log::server(Log::Level::warn, "Client {} missed a heartbeat. {} remaining", performer->URI().path(), MAX_MISSED_HEARTBEATS - performer->get_missed_heartbeats());
					performer->set_heartbeat_inactive();
				}

				if (performer->get_missed_heartbeats() > MAX_MISSED_HEARTBEATS) {
					removed_clients.push_back(performer);
				}
			}
		}

		for (auto client : removed_clients) {
			stage->m_session->hierarchy()->remove_proxy_entity(client);
		}

		if (!stage->is_destroyed()) {
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
				if (this->is_destroyed())
					break;
				this->process_events();
			}
			catch (boost::thread_interrupted) {
				Log::server(Log::Level::debug, "Stage msg event loop exiting.");
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
			Log::server(Log::Level::debug, "Stage timer event loop exiting.");
		}
	}
}