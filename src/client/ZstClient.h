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
#include "../core/ZstSemaphore.h"
#include "../core/ZstMessageSupervisor.hpp"
#include "../core/liasons/ZstPlugLiason.hpp"
#include "../core/liasons/ZstSynchronisableLiason.hpp"
#include "../core/adaptors/ZstTransportAdaptor.hpp"


//Forwards
class ZstSemaphore;
class ZstTCPGraphTransport;
class ZstUDPGraphTransport;
class ZstServiceDiscoveryTransport;
class ZstMessage;
class ZstPerformanceMessage;
class ZstClientSession;
class ZstZMQClientTransport;

//Event loop
struct ZstClientIOLoop {
public:
	ZstClientIOLoop() {};
	void operator()();
	boost::asio::io_service & IO_context();

private:
	boost::asio::io_service m_io;
};


//Typedefs
typedef std::unordered_map<ZstURI, boost::asio::deadline_timer, ZstURIHash> ZstConnectionTimerMap;
typedef std::unique_ptr<ZstConnectionTimerMap> ZstConnectionTimerMapUnique;


class ZstClient : 
	public ZstEventDispatcher<ZstTransportAdaptor*>,
	public ZstTransportAdaptor,
    public ZstHierarchyAdaptor,
	public ZstSynchronisableLiason,
	public ZstPlugLiason
{
public:
	ZstClient();
	~ZstClient();
	void init_client(const char * client_name, bool debug);
	void init_file_logging(const char * log_file_path);
	void destroy();
	
	void process_events();
	void flush();
	
	//Client singleton - should not be accessable outside this interface
	static ZstClient & instance();

	//Stage adaptor overrides
	void on_receive_msg(ZstMessage * msg) override;
	void receive_connection_handshake(ZstPerformanceMessage * msg);

    //Server discovery
    void handle_server_discovery(const std::string & address, const std::string & server_name, int port);
    const ZstServerList & get_discovered_servers();
    
    //Register this endpoint to the stage
    void auto_join_stage(const std::string & name, const ZstTransportSendType & sendtype = ZstTransportSendType::SYNC_REPLY);
	void join_stage(const std::string & stage_address, const ZstTransportSendType & sendtype = ZstTransportSendType::SYNC_REPLY);
	void join_stage_complete(ZstMessageReceipt response);
	void synchronise_graph(const ZstTransportSendType & sendtype = ZstTransportSendType::SYNC_REPLY);
	void synchronise_graph_complete(ZstMessageReceipt response);

	//Leave the stage
	void leave_stage();
	void leave_stage_complete();
    
	//Stage connection status
	bool is_connected_to_stage();
	bool is_connecting_to_stage();
    bool is_init_complete();
	long ping();
	
	//Client modules
	ZstClientSession * session();

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

	//UUIDs
	std::string m_assigned_uuid;
	std::string m_client_name;
    
    //Server discovery
    std::unique_ptr<ZstServiceDiscoveryTransport> m_service_broadcast_transport;
    ZstServerList m_server_beacons;
    bool m_auto_join_stage;
    std::map<std::string, ZstMsgID> m_auto_join_stage_requests;
    ZstMessageSupervisor m_promise_supervisor;

	//P2P Connections
	void start_connection_broadcast(const ZstURI & remote_client_path);
	static void send_connection_broadcast(boost::asio::deadline_timer * t, ZstClient * client, const ZstURI & to, const ZstURI & from, boost::posix_time::milliseconds duration);
	void stop_connection_broadcast(const ZstURI & remote_client_path);
	void listen_to_client(ZstMessage * msg);
	ZstPerformerMap m_active_peer_connections;
	std::unordered_map<ZstURI, ZstMsgID, ZstURIHash> m_pending_peer_connections;
	
	//Client modules
	ZstClientSession * m_session;
	ZstTCPGraphTransport * m_tcp_graph_transport;
	ZstUDPGraphTransport * m_udp_graph_transport;
	ZstZMQClientTransport * m_client_transport;

	//Timers
	boost::thread m_client_timer_thread;
	ZstClientIOLoop m_client_timerloop;
	boost::asio::deadline_timer m_heartbeat_timer;
	ZstConnectionTimerMapUnique m_connection_timers;

	boost::thread m_client_event_thread;
	std::shared_ptr<ZstSemaphore> m_event_condition;
	void transport_event_loop();
};

