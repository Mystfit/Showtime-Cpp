#pragma once

#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

//Showtime API
#include <ZstCore.h>

//Core headers
#include "../core/ZstActor.h"
#include "../core/ZstSemaphore.h"
#include "../core/ZstStageMessage.h"
#include "../core/adaptors/ZstStageTransportAdaptor.hpp"
#include "../core/transports/ZstServiceDiscoveryTransport.h"

//Stage headers
#include "ZstStageSession.h"
#include "ZstZMQServerTransport.h"
#include "ZstWebsocketServerTransport.h"


//Forwards
class ZstSemaphore;


namespace showtime {
	namespace detail {
		class ZstStage : 
			public ZstStageTransportAdaptor,
			public ZstEventDispatcher<ZstTransportAdaptor*>
		{
		public:
			ZstStage();
			~ZstStage();
			void init(const char * stage_name, int port);
			void destroy();
			bool is_destroyed();
			int port();
	
		private:
			bool m_is_destroyed;
			boost::thread m_stage_eventloop_thread;
			boost::thread m_stage_timer_thread;
			int m_port;

			ZstIOLoop m_io;
			//boost::asio::io_context m_io;
			void process_events();
			void timer_loop();
			void event_loop();
			std::shared_ptr<ZstSemaphore> m_event_condition;

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