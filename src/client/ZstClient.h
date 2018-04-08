#pragma once

#include <unordered_map>
#include <string>
#include <czmq.h>

//Showtime API includes
#include <ZstCore.h>

//Showtime Core includes
#include "../core/ZstActor.h"
#include "../core/ZstMessage.h"
#include "../core/ZstMessagePool.hpp"
#include "../core/ZstINetworkInteractor.h"
#include "../core/ZstValue.h"
#include "../core/ZstEventQueue.h"
#include "../core/ZstEventDispatcher.h"

//Client includes
#include "ZstMessageDispatcher.h"
#include "ZstClientEvents.h"
#include "ZstReaper.h"
#include "ZstCableNetwork.h"
#include "ZstHierarchy.h"
#include "ZstCZMQTransportLayer.h"

class ZstClient : public ZstActor, public ZstEventDispatcher, public ZstINetworkInteractor {

public:
	ZstClient();
	~ZstClient();
	void init_client(const char * client_name, bool debug);
	void init_file_logging(const char * log_file_path);
	void destroy() override;
	void process_callbacks() override;
	void flush() override;
	
	//Client singleton - should not be accessable outside this interface
	static ZstClient & instance();

	//Register this endpoint to the stage
	void join_stage(std::string stage_address, bool async = false);
	void join_stage_complete(ZstMessageReceipt response);

	void leave_stage(bool async);
	void leave_stage_complete();
    
	//Stage connection status
	bool is_connected_to_stage();
	bool is_connecting_to_stage();
    bool is_init_complete();
	long ping();
		
	//Callbacks
	ZstEventQueue * client_connected_events();
	ZstEventQueue * client_disconnected_events();
	ZstEventQueue * compute_events();

	//Network interactor implementation
	virtual void enqueue_synchronisable_event(ZstSynchronisable * synchronisable) override;
	void enqueue_synchronisable_deletion(ZstSynchronisable * synchronisable);
	void send_to_performance(ZstPlug * plug);

	//Client modules
	ZstHierarchy * hierarchy();
	ZstCableNetwork * cable_network();
	ZstMessageDispatcher * msg_dispatch();

private:
	//Stage actor
	void start() override;
	void stop() override;
			
	//Message handlers
	int graph_message_handler(zmsg_t * msg);
	void stage_update_handler(ZstMessage * msg);
	
	//Heartbeat timer
	int m_heartbeat_timer_id;
	long m_ping;
	static int s_heartbeat_timer(zloop_t *loop, int timer_id, void *arg);

	//Destruction
	bool m_is_ending;
	bool m_is_destroyed;
    bool m_init_completed;
	bool m_connected_to_stage;
	bool m_is_connecting;

	//UUIDs
	std::string m_assigned_uuid;
	std::string m_client_name;
		
	//Event hooks
	ZstSynchronisableDeferredEvent * m_synchronisable_deferred_event;
	ZstComputeEvent * m_compute_event;
	
	//Events and callbacks
	ZstEventQueue * m_client_connected_event_manager;
	ZstEventQueue * m_client_disconnected_event_manager;
	ZstEventQueue * m_compute_event_manager;
	ZstEventQueue * m_synchronisable_event_manager;

	//Syncronisable reaper
	ZstReaper m_reaper;

	//Client modules
	ZstHierarchy * m_hierarchy;
	ZstCableNetwork * m_cable_network;
	ZstMessageDispatcher * m_msg_dispatch;
	ZstTransportLayer * m_transport;
};
