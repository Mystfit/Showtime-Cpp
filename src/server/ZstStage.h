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
#include "Queue.h"

#define HEARTBEAT_DURATION 1000

class ZstPerformer;
class ZstEntityBase;
class ZstComponent;
class ZstContainer;
class ZstPerformer;
class ZstCable;

class ZstStage : public ZstActor{
public:
    
    ZST_EXPORT ~ZstStage();
	ZST_EXPORT void init();
    ZST_EXPORT static ZstStage* create_stage();

	//Client
	ZST_EXPORT ZstPerformer * get_client_by_URI(const ZstURI & path);
	ZST_EXPORT void destroy_client(ZstPerformer * performer);
	
	//Cables
	ZST_EXPORT ZstCable * create_cable(const ZstURI & a, const ZstURI & b);
	ZST_EXPORT int destroy_cable(const ZstURI & path);
	ZST_EXPORT int destroy_cable(ZstCable * cable);
	ZST_EXPORT int destroy_cable(const ZstURI & output_plug, const ZstURI & input_plug);
	ZST_EXPORT ZstCable * get_cable_by_URI(const ZstURI & uriA, const ZstURI & uriB);
	ZST_EXPORT std::vector<ZstCable*> get_cables_by_URI(const ZstURI & uri);
	ZST_EXPORT std::vector<ZstCable*> get_cables_in_entity(ZstEntityBase * entity);

private:
    ZstStage();

    //Incoming socket handlers
    static int s_handle_router(zloop_t *loop, zsock_t *sock, void *arg);
    static int s_handle_performer_requests(zloop_t *loop, zsock_t *sock, void *arg);

	//Client communication
	void reply_with_signal(zsock_t * socket, ZstMessages::Signal status, ZstPerformer * destination = NULL);
	void send_to_client(zmsg_t * msg, ZstPerformer * destination);

    //Message handlers
    ZstMessages::Signal create_client_handler(const char * buffer, size_t length);
	ZstMessages::Signal destroy_client_handler(const char * buffer, size_t length);

	template <typename T>
	ZstMessages::Signal create_entity_handler(const char * buffer, size_t length, ZstPerformer * performer);
	ZstMessages::Signal destroy_entity_handler(const char * buffer, size_t length);

    ZstMessages::Signal create_entity_template_handler(const char * buffer, size_t length);
    ZstMessages::Signal create_entity_from_template_handler(const char * buffer, size_t length);

	ZstMessages::Signal create_cable_handler(const char * buffer, size_t length);
    ZstMessages::Signal destroy_cable_handler(const char * buffer, size_t length);
	
	//Outgoing event queue
	zmsg_t * create_snapshot();
    void enqueue_stage_update(ZstMessages::Kind k, std::string packed);
    Queue<ZstMessages::MessagePair> m_stage_updates;
	
	int m_update_timer_id;
	static int stage_update_timer_func(zloop_t * loop, int timer_id, void * arg);

	int m_heartbeat_timer_id;
	static int stage_heartbeat_timer_func(zloop_t * loop, int timer_id, void * arg);

	//Stage pipes
	zsock_t *m_performer_router;
	zsock_t *m_graph_update_pub;

	//Client performers
	std::unordered_map<ZstURI, ZstPerformer*> m_clients;

	//Cables
	std::vector<ZstCable*> m_cables;
};

