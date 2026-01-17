#pragma once

#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

//Core headers
#include "../core/ZstActor.h"
#include "../core/ZstSemaphore.h"
#include "../core/ZstEventDispatcher.hpp"
#include "../core/ZstStageMessage.h"
#include "../core/adaptors/ZstStageTransportAdaptor.hpp"
#include "../core/transports/ZstServiceDiscoveryTransport.h"

//Stage headers
#include "ZstStageSession.h"
#include "ZstWebsocketServerTransport.h"
#include "ZstZMQServerTransport.h"

//Showtime API
#include <showtime/ZstCore.h>


//Forwards
class ZstSemaphore;


namespace showtime {
	namespace detail {
		class ZstStage :
			public ZstStageTransportAdaptor,
			public ZstEventDispatcher<ZstLogAdaptor>
		{
		public:
			ZstStage();
			~ZstStage();
			void init(const char* server_name = "stage", int port = -1, bool unlisted = false);
			void destroy();
			bool is_destroyed();
			void start_broadcasting(const char* stage_name);
			void stop_broadcasting();
			void send_shutdown_signal();
			int port();

			// Session management
			bool save_session(const std::string& filepath);
			bool load_session(const std::string& filepath);

			// Offline entity configuration
			void set_preserve_entities_on_disconnect(bool preserve);
			bool get_preserve_entities_on_disconnect() const;

			// Auto-load session
			void set_auto_load_session(const std::string& filepath);

		private:
			bool m_is_destroyed;
			bool m_preserve_entities_on_disconnect = false;
			std::string m_auto_load_session_path;
			boost::thread m_stage_eventloop_thread;
			boost::thread m_stage_timer_thread;

			ZstIOLoop m_io;
			//boost::asio::io_context m_io;
			void process_events();
			void timer_loop();
			void event_loop();
			std::shared_ptr<std::condition_variable> m_event_condition;
			std::mutex m_mtx;

			//Timers
			static void stage_heartbeat_timer(boost::asio::deadline_timer * t, ZstStage * stage, boost::posix_time::milliseconds duration);
			boost::asio::deadline_timer m_heartbeat_timer;

			//Modules
			std::shared_ptr<ZstStageSession> m_session;
	
			//Transports
			std::shared_ptr<ZstZMQServerTransport> m_router_transport;
			std::shared_ptr<ZstWebsocketServerTransport> m_websocket_transport;
			std::shared_ptr<ZstServiceDiscoveryTransport> m_service_broadcast_transport;
		};
	}
}