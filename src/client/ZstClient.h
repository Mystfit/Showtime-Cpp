#pragma once

#include <unordered_map>
#include <string>
#include <czmq.h>

//Showtime API includes
#include <ZstCore.h>

//Showtime Core includes
#include "../core/ZstActor.h"
#include "../core/ZstMessage.h"
#include "../core/ZstMessagePool.h"
#include "../core/ZstINetworkInteractor.h"
#include "../core/ZstValue.h"
#include "../core/ZstEventQueue.h"
#include "../core/ZstEventDispatcher.h"

//Client includes
#include "ZstClientEvents.h"
#include "ZstReaper.h"

class ZstClient : public ZstActor, public ZstINetworkInteractor, public ZstEventDispatcher {

public:
	ZstClient();
	~ZstClient();
	void init_client(const char * client_name, bool debug);
	void init_file_logging(const char * log_file_path);
	void destroy() override;
	void process_callbacks() override;
	
	//CLient singleton - should not be accessable outside this interface
	static ZstClient & instance();

	//Register this endpoint to the stage
	void join_stage(std::string stage_address, bool async = false);
	void leave_stage(bool immediately = false);
    void synchronise_graph(bool async = false);
    
	//Stage connection status
	bool is_connected_to_stage();
	bool is_connecting_to_stage();
    bool is_init_complete();
	long ping();
		
	//Callbacks
	ZstEventQueue & client_connected_events();
	ZstEventQueue & client_disconnected_events();
	ZstEventQueue & compute_events();

	//Network interactor implementation
	virtual void enqueue_synchronisable_event(ZstSynchronisable * synchronisable) override;

private:
	//Stage actor
	void start() override;
	void stop() override;
			
	//Message handlers
	int graph_message_handler(zmsg_t * msg);
	void stage_update_handler(ZstMessage * msg);
	void connect_client_handler(const char * endpoint_ip, const char * output_plug);
	
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
	zuuid_t * m_startup_uuid;
	std::string m_assigned_uuid;
	std::string m_client_name;
		
	//Event hooks
	void flush_events();
	ZstSynchronisableDeferredEvent * m_synchronisable_deferred_event;
	ZstComputeEvent * m_compute_event;

	//Stage communication
	void join_stage_complete(ZstMsgKind status);
	void synchronise_graph_complete(ZstMsgKind status);
	void leave_stage_complete();
	
	//Events and callbacks
	ZstEventQueue m_client_connected_event_manager;
	ZstEventQueue m_client_disconnected_event_manager;
	ZstEventQueue m_compute_event_manager;
	ZstEventQueue m_synchronisable_event_manager;
	
	//Addresses
	std::string m_stage_addr = "127.0.0.1";
};
