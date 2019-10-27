#pragma once

//std lib includes
#include <unordered_map>
#include <string>
#include <mutex>
#include <set>
#include <memory>

//Boost includes
#include <boost/asio.hpp>
#include <boost/thread.hpp>

//Showtime API includes
#include <ZstCore.h>

//Showtime Core includes
#include <schemas/stage_message_generated.h>
#include "../core/ZstSemaphore.h"
#include "../core/ZstZMQRefCounter.h"
#include "../core/ZstIOLoop.h"
#include "../core/ZstMessageSupervisor.hpp"
#include "../core/liasons/ZstSynchronisableLiason.hpp"
#include "../core/liasons/ZstPlugLiason.hpp"
#include "../core/adaptors/ZstStageTransportAdaptor.hpp"
#include "../core/adaptors/ZstGraphTransportAdaptor.hpp"
#include "../core/transports/ZstTransportHelpers.h"

#include "ZstZMQClientTransport.h"
#include "../core/transports/ZstServiceDiscoveryTransport.h"
#include "../core/transports/ZstTCPGraphTransport.h"
#ifdef ZST_BUILD_DRAFT_API
#include "../core/transports/ZstUDPGraphTransport.h"
#endif

//Showtime client includes
#include "ZstClientSession.h"

namespace showtime {

	//Typedefs
	typedef std::unordered_map<ZstURI, boost::asio::deadline_timer, ZstURIHash> ZstConnectionTimerMap;
	typedef std::unique_ptr<ZstConnectionTimerMap> ZstConnectionTimerMapUnique;

    namespace client {
        class ZstClient :
			public ZstEventDispatcher< std::shared_ptr<ZstStageTransportAdaptor> >,
            public ZstEventDispatcher< std::shared_ptr<ZstConnectionAdaptor> >,
            public ZstStageTransportAdaptor,
            public ZstGraphTransportAdaptor,
            public ZstHierarchyAdaptor,
            public ZstPlugLiason,
            public ZstSynchronisableLiason
        {
        public:
            ZstClient(ShowtimeClient* api);
            ~ZstClient();
            void init_client(const char * client_name, bool debug);
            void init_file_logging(const char * log_file_path);
            void destroy();
            
            void process_events();
            void flush();

            //Stage adaptor overrides
            void on_receive_msg(const ZstStageMessage * msg) override;
            void on_receive_msg(const ZstPerformanceMessage * msg) override;

            void connection_handshake_handler(const ZstPerformanceMessage * msg);

            //Server discovery
            void handle_server_discovery(const std::string & address, const std::string & server_name, int port);
            const ZstServerList & get_discovered_servers();
            
            //Register this endpoint to the stage
            void auto_join_stage(const std::string & name, const ZstTransportRequestBehaviour & sendtype = ZstTransportRequestBehaviour::SYNC_REPLY);
            void join_stage(const ZstServerAddress & stage_address, const ZstTransportRequestBehaviour & sendtype = ZstTransportRequestBehaviour::SYNC_REPLY);
            void join_stage_complete(const ZstServerAddress & server_address, ZstMessageReceipt response);
            void synchronise_graph(const ZstTransportRequestBehaviour & sendtype = ZstTransportRequestBehaviour::SYNC_REPLY);
            void synchronise_graph_complete(ZstMessageReceipt response);

            //Leave the stage
            void leave_stage();
            void leave_stage_complete();
            
            //Stage connection status
            const ZstServerAddress & connected_server();
            bool is_connected_to_stage();
            bool is_connecting_to_stage();
            bool is_init_complete();
            long ping();
            
            //Client modules
			std::shared_ptr<ZstClientSession> session();

        private:
            //Heartbeat timer
            long m_ping;
            static void heartbeat_timer(boost::asio::deadline_timer * t, ZstClient * client, boost::posix_time::milliseconds duration);
            
            //Flags
            void set_is_ending(bool value);
            bool m_is_ending;

            void set_is_destroyed(bool value);
            bool m_is_destroyed;

            void set_init_completed(bool value);
            bool m_init_completed;

            void set_connected_to_stage(bool value);
            bool m_connected_to_stage;

            void set_is_connecting(bool value);
            bool m_is_connecting;

            //Module adaptor overrides
            virtual void on_entity_arriving(ZstEntityBase * entity) override;
            virtual void on_performer_arriving(ZstPerformer* performer) override;
            
            // Plug initialization
            void init_arriving_plug(ZstPlug* plug);

            //Names
            std::string m_client_name;
            
            //Server discovery
            std::shared_ptr<ZstServiceDiscoveryTransport> m_service_broadcast_transport;
            ZstServerList m_server_beacons;
            bool m_auto_join_stage;
            std::map<std::string, ZstMsgID> m_auto_join_stage_requests;
            ZstMessageSupervisor m_promise_supervisor;
            ZstServerAddress m_connected_server;

            // Message handlers
            void start_connection_broadcast_handler(const ClientGraphHandshakeStart* request);
            void stop_connection_broadcast_handler(const ClientGraphHandshakeStop* request);
            void listen_to_client_handler(const ClientGraphHandshakeListen* request, const ZstMsgID & request_id);
            void server_discovery_handler(const ServerBeacon* request);

            static void send_connection_broadcast(boost::asio::deadline_timer * t, ZstClient * client, const ZstURI & to, const ZstURI & from, boost::posix_time::milliseconds duration);
            ZstPerformerMap m_active_peer_connections;
            std::unordered_map<ZstURI, ZstMsgID, ZstURIHash> m_pending_peer_connections;
            
            //Client modules
            std::shared_ptr<ZstClientSession> m_session;

            //Transports
            std::shared_ptr<ZstTCPGraphTransport> m_tcp_graph_transport;
        #ifdef ZST_BUILD_DRAFT_API
            std::shared_ptr<ZstUDPGraphTransport> m_udp_graph_transport;
        #endif
            std::shared_ptr<ZstZMQClientTransport> m_client_transport;

            //Timers
            boost::thread m_client_timer_thread;
            ZstIOLoop m_client_timerloop;
            boost::asio::deadline_timer m_heartbeat_timer;
            ZstConnectionTimerMapUnique m_connection_timers;

            boost::thread m_client_event_thread;
            std::shared_ptr<ZstSemaphore> m_event_condition;
            void transport_event_loop();
            
            //Api object
            ShowtimeClient* m_api;
        };
    }
}
