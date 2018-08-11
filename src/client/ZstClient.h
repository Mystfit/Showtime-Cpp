#pragma once

//std lib includes
#include <unordered_map>
#include <string>
#include <mutex>

//Showtime API includes
#include <ZstCore.h>

//Showtime Core includes
#include "../core/ZstActor.h"
#include "../core/ZstMessage.h"
#include "../core/ZstMessagePool.hpp"
#include "../core/ZstValue.h"
#include "../core/adaptors/ZstTransportAdaptor.hpp"

//Showtime client includes
#include "ZstClientSession.h"
#include "ZstGraphTransport.h"
#include "ZstClientTransport.h"


class ZstClient : 
	public ZstEventDispatcher<ZstTransportAdaptor*>,
	public ZstTransportAdaptor,
	public ZstSynchronisableLiason
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
	void receive_connection_handshake(ZstMessage * msg);

	//Register this endpoint to the stage
	void join_stage(std::string stage_address, const ZstTransportSendType & sendtype = ZstTransportSendType::SYNC_REPLY);
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
	int m_heartbeat_timer_id;
	long m_ping;
	void heartbeat_timer();
		
	//Destruction
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

	//UUIDs
	std::string m_assigned_uuid;
	std::string m_client_name;

	//P2P Connections
	void start_connection_broadcast(const ZstURI & remote_client_path);
	void stop_connection_broadcast(const ZstURI & remote_client_path);
	void listen_to_client(const ZstMessage * msg);
	ZstPerformerMap m_active_peer_connections;
	std::unordered_map<ZstURI, ZstMsgID, ZstURIHash> m_pending_peer_connections;
	std::unordered_map<ZstURI, int, ZstURIHash> m_connection_timers;
	
	//Client modules
	ZstClientSession * m_session;
	ZstGraphTransport * m_graph_transport;
	ZstClientTransport * m_client_transport;
	ZstActor * m_actor;
	std::mutex m_event_loop_mutex;
};
