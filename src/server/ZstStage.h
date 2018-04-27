#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include "czmq.h"

//Showtime API
#include <ZstCore.h>

//Core headers
#include "../core/ZstActor.h"
#include "../core/ZstMessagePool.hpp"
#include "../core/ZstCZMQMessage.h"

//Stage headers
#include "ZstPerformerStageProxy.h"

#define HEARTBEAT_DURATION 1000
#define MAX_MISSED_HEARTBEATS 10
#define STAGE_MESSAGE_POOL_BLOCK 512

class ZstEntityBase;
class ZstPerformer;
class ZstMessage;
class ZstCable;

typedef std::unordered_map<ZstURI, ZstPerformerStageProxy*, ZstURIHash> ZstClientMap;
typedef std::unordered_map<std::string, ZstPerformerStageProxy*> ZstClientSocketMap;

class ZstStage : public ZstActor {
public:
	ZstStage();
	~ZstStage();
	void init(const char * stage_name);
	void destroy() override;
	bool is_destroyed();

	//Client
	ZstPerformer * get_client(const ZstURI & path);
	ZstPerformer * get_client_from_socket_id(const std::string & socket_id);
	std::string get_socket_ID(const ZstPerformer * performer);

	void destroy_client(ZstPerformer * performer);
	
	//Cables
	ZstCable * create_cable(const ZstCable & cable);
	ZstCable * create_cable(const ZstURI & input_URI, const ZstURI & output_URI);
	int destroy_cable(const ZstURI & path);
	int destroy_cable(ZstCable * cable);
	int destroy_cable(const ZstURI & output_plug, const ZstURI & input_plug);
	ZstCable * get_cable_by_URI(const ZstURI & output, const ZstURI & input);
	std::vector<ZstCable*> find_cables(const ZstURI & uri);
	std::vector<ZstCable*> get_cables_in_entity(ZstEntityBase * entity);

private:
	bool m_is_destroyed;

    //Incoming socket handlers
	static int s_handle_router(zloop_t *loop, zsock_t *sock, void *arg);

	//Client communication
	void send_to_client(ZstCZMQMessage * msg, ZstPerformer * destination);

    //Message handlers
    ZstMessage * create_client_handler(std::string sender_identity, ZstMessage * msg);
	ZstMessage * destroy_client_handler(ZstPerformer * performer);

	template <typename T>
	ZstMessage * create_entity_handler(ZstMessage * msg, ZstPerformer * performer);
	ZstMessage * destroy_entity_handler(ZstMessage * msg);

	ZstMessage * create_entity_template_handler(ZstMessage * msg);
	ZstMessage * create_entity_from_template_handler(ZstMessage * msg);

	ZstMessage * create_cable_handler(ZstMessage * msg);
	ZstMessage * destroy_cable_handler(ZstMessage * msg);
	
	//Outgoing events
	ZstMessage * create_snapshot(ZstPerformer * client);
    void publish_stage_update(ZstCZMQMessage * msg);

	int m_heartbeat_timer_id;
	static int stage_heartbeat_timer_func(zloop_t * loop, int timer_id, void * arg);

	//Stage pipes
	zsock_t *m_performer_router;
	zsock_t *m_graph_update_pub;

	//Client performers
	ZstClientMap m_clients;
	ZstClientSocketMap m_client_socket_index;
	
	//Cables
	ZstCableList m_cables;

	//Messages
	ZstMessagePool<ZstCZMQMessage> * m_message_pool;
	ZstMessagePool<ZstCZMQMessage> * msg_pool();
};
