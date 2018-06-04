#pragma once

//std lib includes
#include <unordered_map>
#include <string>

//Showtime API includes
#include <ZstCore.h>

//Showtime Core includes
#include "../core/ZstActor.h"
#include "../core/ZstMessage.h"
#include "../core/ZstMessagePool.hpp"
#include "../core/ZstValue.h"

//Showtime client includes
#include "ZstReaper.h"
#include "ZstMessageDispatcher.h"
#include "ZstReaper.h"
#include "ZstClientSession.h"
#include "ZstCZMQTransportLayer.h"
#include "../core/adaptors/ZstStageDispatchAdaptor.hpp"
#include "../core/adaptors/ZstPerformanceDispatchAdaptor.hpp"

class ZstClient : 
	public ZstEventDispatcher<ZstStageDispatchAdaptor*>,
	public ZstStageDispatchAdaptor,
	public ZstPerformanceDispatchAdaptor
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
	void on_receive_from_stage(ZstStageMessage * msg) override;
	void on_receive_from_performance(ZstPerformanceMessage * msg) override;

	//Register this endpoint to the stage
	void join_stage(std::string stage_address, bool async = false);
	void join_stage_complete(ZstMessageReceipt response);
	void synchronise_graph(bool async = false);
	void synchronise_graph_complete(ZstMessageReceipt response);

	void leave_stage(bool async);
	void leave_stage_complete();
    
	//Stage connection status
	bool is_connected_to_stage();
	bool is_connecting_to_stage();
    bool is_init_complete();
	long ping();

	//Client modules
	ZstMessageDispatcher * msg_dispatch();
	ZstClientSession * session();

private:	
	//Heartbeat timer
	int m_heartbeat_timer_id;
	long m_ping;
	void heartbeat_timer();
		
	//Destruction
	bool m_is_ending;
	bool m_is_destroyed;
    bool m_init_completed;
	bool m_connected_to_stage;
	bool m_is_connecting;

	//UUIDs
	std::string m_assigned_uuid;
	std::string m_client_name;

	//P2P Connections
	void start_connection_broadcast(ZstPerformer * local_client, ZstPerformer * remote_client);
	void stop_connection_broadcast(ZstPerformer * remote_client);
	std::unordered_set<ZstURI, ZstURIHash> m_pending_peer_connections;
	std::unordered_map<ZstURI, int, ZstURIHash> m_connection_timers;
	
	//Client modules
	ZstClientSession * m_session;
	ZstMessageDispatcher * m_msg_dispatch;
	ZstCZMQTransportLayer * m_transport;
};
