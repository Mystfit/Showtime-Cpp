#pragma once
#include "czmq.h"
#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <algorithm>
#include <regex>
#include "ZstActor.h"
#include "ZstExports.h"
#include "ZstMessages.h"
#include "ZstURI.h"
#include "ZstPlugRef.h"
#include "ZstPerformerRef.h"
#include "ZstEndpointRef.h"
#include "Showtime.h"
#include "Queue.h"

class ZstStage : public ZstActor{
public:
    //int dealer_port = 6000;
    //int router_port = 6001;
    
    ZST_EXPORT ~ZstStage();
	ZST_EXPORT void init();
    ZST_EXPORT static ZstStage* create_stage();

	ZST_EXPORT std::vector<ZstPlugRef*> get_all_plug_refs();
	ZST_EXPORT std::vector<ZstPerformerRef*> get_all_performer_refs();
    ZST_EXPORT ZstPerformerRef * get_performer_ref_by_name(std::string performer_name);
	ZST_EXPORT ZstEndpointRef * get_performer_endpoint(ZstPerformerRef * performer);
    
private:
    ZstStage();

	ZstURI m_stage_identity;

    //Stage pipes
    zsock_t *m_performer_router;
    zsock_t *m_performer_requests;
    zsock_t *m_graph_update_pub;

    //Incoming router socket handler
    static int s_handle_router(zloop_t *loop, zsock_t *sock, void *arg);
    static int s_handle_performer_requests(zloop_t *loop, zsock_t *sock, void *arg);

	//Replies
	void reply_with_signal(zsock_t * socket, ZstMessages::Signal status, ZstEndpointRef * destination = NULL);
	void send_to_endpoint(zmsg_t * msg, ZstEndpointRef * destination);

    //Message handlers
	//Rep
	void register_endpoint_handler(zsock_t * socket, zmsg_t * msg);
	void endpoint_heartbeat_handler(zsock_t * socket, zmsg_t * msg);
	void register_performer_handler(zsock_t * socket, zmsg_t * msg);
	void register_plug_handler(zsock_t * socket, zmsg_t * msg);
	void destroy_plug_handler(zsock_t * socket, zmsg_t * msg);

	//Router
    void connect_plugs_handler(zsock_t * socket, zmsg_t * msg);
    
    //Graph storage
	ZstEndpointRef * create_endpoint(std::string starting_uuid, std::string endpoint);
	ZstEndpointRef * get_endpoint_ref_by_UUID(std::string uuid);
	void destroy_endpoint(ZstEndpointRef* endpoint);
	std::map<std::string, ZstEndpointRef*> m_endpoint_refs;

	//Plug connections
    int connect_plugs(ZstURI output_plug, ZstURI input_plug);

	//Queued stage events
	std::vector<ZstEvent> create_snapshot();
	void enqueue_stage_update(ZstEvent e);
	Queue<ZstEvent> m_stage_updates;
	int m_update_timer_id;
	static int stage_update_timer_func(zloop_t * loop, int timer_id, void * arg);
	
	//Database handle
	sqlite3 * db;
};

