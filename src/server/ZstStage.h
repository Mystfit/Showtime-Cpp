#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <regex>
#include "czmq.h"

#include <ZstCore.h>

//Core headers
#include "../core/ZstActor.h"
#include "../core/Queue.h"

#define HEARTBEAT_DURATION 1000
#define STAGE_MESSAGE_POOL_BLOCK 512

class ZstEntityBase;
class ZstPerformer;
class ZstMessage;
class ZstMessagePool;
class ZstCable;

class ZstStage : public ZstActor{
public:
	ZstStage();
	~ZstStage();
	void init();

	//Client
	ZstPerformer * get_client(const ZstURI & path);
	ZstPerformer * get_client_from_socket_id(const std::string & socket_id);
	std::string get_socket_ID(const ZstPerformer * performer);

	void destroy_client(ZstPerformer * performer);
	
	//Cables
	ZstCable * create_cable(const ZstURI & a, const ZstURI & b);
	int destroy_cable(const ZstURI & path);
	int destroy_cable(ZstCable * cable);
	int destroy_cable(const ZstURI & output_plug, const ZstURI & input_plug);
	ZstCable * get_cable_by_URI(const ZstURI & uriA, const ZstURI & uriB);
	std::vector<ZstCable*> find_cables(const ZstURI & uri);
	std::vector<ZstCable*> get_cables_in_entity(ZstEntityBase * entity);

private:

    //Incoming socket handlers
    static int s_handle_router(zloop_t *loop, zsock_t *sock, void *arg);

	//Client communication
	void send_to_client(ZstMessage * msg, ZstPerformer * destination);

    //Message handlers
    ZstMessage * create_client_handler(std::string sender_identity, ZstMessage * msg);
	void destroy_client_handler(ZstPerformer * performer);

	template <typename T>
	ZstMessage * create_entity_handler(ZstMessage * msg, ZstPerformer * performer);
	ZstMessage * destroy_entity_handler(ZstMessage * msg);

	ZstMessage * create_entity_template_handler(ZstMessage * msg);
	ZstMessage * create_entity_from_template_handler(ZstMessage * msg);

	ZstMessage * create_cable_handler(ZstMessage * msg);
	ZstMessage * destroy_cable_handler(ZstMessage * msg);
	
	//Outgoing event queue
	void send_snapshot(ZstPerformer * client);
    void publish_stage_update(ZstMessage * msg);
    Queue<ZstMessage*> m_stage_updates;	

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

	//Messages
	ZstMessagePool * m_message_pool;
	ZstMessagePool * msg_pool();
};
