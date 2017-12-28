#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <algorithm>
#include <regex>
#include "czmq.h"
#include "ZstURI.h"

//Core headers
#include "../core/ZstActor.h"
#include "../core/ZstMessage.h"
#include "../core/Queue.h"

#define HEARTBEAT_DURATION 1000

class ZstEntityBase;
class ZstPerformer;
class ZstCable;

struct MessagePair {
	ZstMessage::Kind kind;
	zframe_t * packed;
};

class ZstStage : public ZstActor{
public:
    
	~ZstStage();
	void init();
    static ZstStage* create_stage();

	//Client
	ZstPerformer * get_client(const ZstURI & path);
	ZstPerformer * get_client(const std::string & socket_id);
	std::string get_socket_ID(const ZstPerformer * performer);

	void destroy_client(ZstPerformer * performer);
	
	//Cables
	ZstCable * create_cable_ptr(const ZstURI & a, const ZstURI & b);
	int destroy_cable(const ZstURI & path);
	int destroy_cable(ZstCable * cable);
	int destroy_cable(const ZstURI & output_plug, const ZstURI & input_plug);
	ZstCable * get_cable_by_URI(const ZstURI & uriA, const ZstURI & uriB);
	std::vector<ZstCable*> find_cables(const ZstURI & uri);
	std::vector<ZstCable*> get_cables_in_entity(ZstEntityBase * entity);

private:
    ZstStage();

    //Incoming socket handlers
    static int s_handle_router(zloop_t *loop, zsock_t *sock, void *arg);

	//Client communication
	void reply_with_signal(zsock_t * socket, ZstMessage::Signal status, ZstPerformer * destination);
	void send_to_client(zmsg_t * msg, ZstPerformer * destination);

    //Message handlers
	ZstMessage::Signal signal_handler(zframe_t * frame, ZstPerformer * sender);
    ZstMessage::Signal create_client_handler(std::string sender, zframe_t * frame);
	ZstMessage::Signal destroy_client_handler(ZstPerformer * performer);

	template <typename T>
	ZstMessage::Signal create_entity_handler(zframe_t * frame, ZstPerformer * performer);
	ZstMessage::Signal destroy_entity_handler(zframe_t * frame);

    ZstMessage::Signal create_entity_template_handler(zframe_t * frame);
    ZstMessage::Signal create_entity_from_template_handler(zframe_t * frame);

	ZstMessage::Signal create_cable_handler(zframe_t * frame);
    ZstMessage::Signal destroy_cable_handler(zframe_t * frame);
	
	//Outgoing event queue
	zmsg_t * create_snapshot();
    void enqueue_stage_update(ZstMessage::Kind k, zframe_t * frame);
    Queue<MessagePair> m_stage_updates;
	
	int m_update_timer_id;
	static int stage_update_timer_func(zloop_t * loop, int timer_id, void * arg);

	int m_heartbeat_timer_id;
	static int stage_heartbeat_timer_func(zloop_t * loop, int timer_id, void * arg);

	//Stage pipes
	zsock_t *m_performer_router;
	zsock_t *m_graph_update_pub;

	//Client performers
	std::unordered_map<ZstURI, ZstPerformer*> m_clients;
	std::unordered_map<std::string, ZstPerformer*> m_client_socket_index;
	
	//Cables
	std::vector<ZstCable*> m_cables;
};

