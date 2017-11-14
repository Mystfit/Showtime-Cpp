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
#include "ZstCable.h"
#include "ZstEventWire.h"
#include "ZstPlugRef.h"
#include "ZstEndpointRef.h"
#include "ZstEntityWire.h"
#include "Queue.h"

#define HEARTBEAT_DURATION 1000

class ZstStage : public ZstActor{
public:
    
    ZST_EXPORT ~ZstStage();
	ZST_EXPORT void init();
    ZST_EXPORT static ZstStage* create_stage();

	ZST_EXPORT ZstEndpointRef * get_endpoint_ref_by_UUID(const char * uuid);

	ZST_EXPORT std::vector<ZstPlugRef*> get_all_plug_refs();
    ZST_EXPORT std::vector<ZstPlugRef*> get_all_plug_refs(ZstEndpointRef * endpoint);
	ZST_EXPORT ZstEndpointRef * get_plug_endpoint(ZstPlugRef * plug);
	ZST_EXPORT ZstPlugRef* get_plug_by_URI(ZstURI uri);

	ZST_EXPORT std::vector<ZstProxy*> get_all_entity_proxies();
	ZST_EXPORT std::vector<ZstProxy*> get_all_entity_proxies(ZstEndpointRef* endpoint);
    
    ZST_EXPORT ZstEndpointRef * get_entity_endpoint(ZstEntityBase * entity);
    ZST_EXPORT ZstProxy* get_entity_proxy_by_URI(const ZstURI & uri);
	
	ZST_EXPORT ZstCable * get_cable_by_URI(const ZstURI & uriA, const ZstURI & uriB);
	ZST_EXPORT std::vector<ZstCable*> get_cables_by_URI(const ZstURI & uri);

    
private:
    ZstStage();

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
    ZstMessages::Signal create_endpoint_handler(zsock_t * socket, zmsg_t * msg);
	ZstMessages::Signal endpoint_heartbeat_handler(zmsg_t * msg);
	ZstMessages::Signal register_entity_recipe_handler(ZstEntityWire & entity, ZstEndpointRef * endpoint);
	ZstMessages::Signal create_plug_handler(zmsg_t * msg);
    ZstMessages::Signal create_entity_template_handler(zmsg_t * msg);
    ZstMessages::Signal create_entity_handler(ZstEntityWire & entity, ZstEndpointRef * endpoint);
    ZstMessages::Signal create_entity_from_template_handler(zmsg_t * msg);
	ZstMessages::Signal destroy_plug_handler(zmsg_t * msg);
	ZstMessages::Signal destroy_entity_handler(zmsg_t * msg);
    ZstMessages::Signal create_cable_handler(zmsg_t * msg);
    ZstMessages::Signal destroy_cable_handler(zmsg_t * msg);

    //Graph storage
	ZstEndpointRef * create_endpoint(std::string starting_uuid, std::string endpoint);
	void destroy_endpoint(ZstEndpointRef* endpoint);
	std::map<std::string, ZstEndpointRef*> m_endpoint_refs;

	//Plug connections
    int connect_cable(ZstPlugRef * output_plug, ZstPlugRef * input_plug);
    int destroy_cable(const ZstURI & uri);
	int destroy_cable(ZstCable * cable);
	int destroy_cable(const ZstURI & output_plug, const ZstURI & input_plug);
	std::vector<ZstCable*> m_cables;

	//Queued stage events
	std::vector<ZstEventWire> create_snapshot();
	void enqueue_stage_update(ZstEvent e);
	Queue<ZstEvent> m_stage_updates;
	
	int m_update_timer_id;
	static int stage_update_timer_func(zloop_t * loop, int timer_id, void * arg);

	int m_heartbeat_timer_id;
	static int stage_heartbeat_timer_func(zloop_t * loop, int timer_id, void * arg);

};

