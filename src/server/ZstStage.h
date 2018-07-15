#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cf/cfuture.h>
#include <cf/time_watcher.h>
#include "czmq.h"

//Showtime API
#include <ZstCore.h>

//Core headers
#include "../core/ZstActor.h"
#include "../core/ZstMessagePool.hpp"
#include "../core/ZstStageMessage.h"

//Stage headers
#include "ZstPerformerStageProxy.h"


//TODO: Temp defines to get stage functioning with broadcast connections
//----------------------------------------------------------------------
struct ZstTimeoutException : std::runtime_error {
	using std::runtime_error::runtime_error;
};
typedef cf::promise<ZstMsgKind> MessagePromise;
typedef cf::future<ZstMsgKind> MessageFuture;
//----------------------------------------------------------------------
//TODO: end

#define HEARTBEAT_DURATION 1000
#define MAX_MISSED_HEARTBEATS 10
#define STAGE_MESSAGE_POOL_BLOCK 512

class ZstEntityBase;
class ZstPerformer;
class ZstCable;

typedef std::unordered_map<ZstURI, ZstPerformerStageProxy*, ZstURIHash> ZstClientMap;
typedef std::unordered_map<std::string, ZstPerformerStageProxy*> ZstClientSocketMap;

class ZstStage : public ZstActor {
public:
	ZstStage();
	~ZstStage();
	void init_stage(const char * stage_name);
	void destroy() override;
	bool is_destroyed();

	//Client
	ZstPerformerStageProxy * get_client(const ZstURI & path);
	ZstPerformerStageProxy * get_client_from_socket_id(const std::string & socket_id);
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
	void send_to_client(ZstStageMessage * msg, ZstPerformer * destination);

	//TODO: Replace with abstract future message handler class
	MessageFuture register_response_message(ZstStageMessage * msg);
	void process_client_response(ZstStageMessage * msg);

    //Message handlers
    ZstStageMessage * create_client_handler(std::string sender_identity, ZstStageMessage * msg);
	ZstStageMessage * destroy_client_handler(ZstPerformer * performer);

	template <typename T>
	ZstStageMessage * create_entity_handler(ZstStageMessage * msg, ZstPerformer * performer);
	ZstStageMessage * destroy_entity_handler(ZstStageMessage * msg);

	ZstStageMessage * create_entity_template_handler(ZstStageMessage * msg);
	ZstStageMessage * create_entity_from_template_handler(ZstStageMessage * msg);

	ZstStageMessage * create_cable_handler(ZstStageMessage * msg, ZstPerformer * performer);
	ZstStageMessage * create_cable_complete_handler(ZstCable * cable);
	ZstStageMessage * destroy_cable_handler(ZstStageMessage * msg);

	ZstStageMessage * complete_client_connection_handler(ZstStageMessage * msg, ZstPerformerStageProxy * input_client);
	
	//Outgoing events
	ZstStageMessage * synchronise_client_graph(ZstPerformer * client);
    void publish_stage_update(ZstStageMessage * msg);

	int m_heartbeat_timer_id;
	void stage_heartbeat_timer_func();

	//P2P client connections
	void connect_clients(ZstMsgID response_id, ZstPerformerStageProxy * output_client, ZstPerformerStageProxy * input_client);
	
	//Stage pipes
	zsock_t * m_performer_router;
	zsock_t * m_graph_update_pub;

	//Client performers
	ZstClientMap m_clients;
	ZstClientSocketMap m_client_socket_index;

	//Cables
	ZstCableList m_pending_cables;
	ZstCableList m_cables;

	//Messages
	ZstMessagePool<ZstStageMessage> m_message_pool;
	ZstMessagePool<ZstStageMessage> & msg_pool();

	//TODO: Replace with abstract future message handler class
	std::unordered_map<ZstMsgID, MessagePromise > m_promise_messages;
	cf::time_watcher m_timeout_watcher;
};
