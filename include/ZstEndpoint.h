#pragma once

#include <unordered_map>
#include <vector>
#include <czmq.h>
#include <string>
#include <msgpack.hpp>
#include "ZstActor.h"
#include "ZstExports.h"
#include "Queue.h"
#include "ZstPlug.h"
#include "ZstMessages.h"
#include "ZstCable.h"
#include "ZstCallbackQueue.h"
#include "entities/ZstEntityBase.h"

//Forward declarations
class ZstValue;
class ZstURI;
class ZstPerformer;
class ZstComponent;
class ZstProxyComponent;
class ZstReaper;

class ZstEndpoint : public ZstActor {
public:
	friend class Showtime;
	ZstEndpoint();
	~ZstEndpoint();
	ZST_EXPORT void init(const char * performer_name);
	ZST_EXPORT void destroy();
	ZST_EXPORT void process_callbacks();

	//Send/receive
	void send_to_graph(zmsg_t * msg);
	zmsg_t * receive_from_graph();

	//Register this endpoint to the stage
	void register_endpoint_to_stage(std::string stage_address);
	ZST_EXPORT const char * get_endpoint_UUID() const;

	//Stage connection status
	bool is_connected_to_stage();

	//Lets the stage know we want a full snapshot of the current performance
	void signal_sync();
    
    //Updates stage with our recipes
    void sync_recipes();

	//Entity recipes
	int register_entity_type(ZstEntityBase * entity);
    int unregister_entity_type(ZstEntityBase * entity);
    int run_entity_template(ZstEntityBase * entity);
    
    //Entities
	int register_entity(ZstEntityBase * entity);
	int destroy_entity(ZstEntityBase * entity);
    void create_proxy_entity(const ZstURI & path, bool is_template);
	ZstEntityBase * get_entity_by_URI(const ZstURI & uri) const;
	ZstPlug * get_plug_by_URI(const ZstURI & uri) const;
    ZstEntityBase * get_root() const;

	//Plugs
	template<typename T>
	ZST_EXPORT T* create_plug(ZstComponent * owner, const char * name, ZstValueType val_type);
	ZST_EXPORT int destroy_plug(ZstPlug * plug);
    void enqueue_compute(ZstInputPlug * plug);

	//Cables
	int connect_cable(const ZstURI & a, const ZstURI & b);
	int destroy_cable(const ZstURI & a, const ZstURI & b);
	ZST_EXPORT std::vector<ZstCable*> get_cables_by_URI(const ZstURI & uri);
	ZST_EXPORT ZstCable * get_cable_by_URI(const ZstURI & uriA, const ZstURI & uriB);
	void remove_cable(ZstCable * cable);
	
    //Utility
	ZST_EXPORT int ping_stage();

	//Plug callbacks
	ZstCallbackQueue<ZstEntityEventCallback, ZstEntityBase*> * entity_arriving_events();
	ZstCallbackQueue<ZstEntityEventCallback, ZstEntityBase*> * entity_leaving_events();
    ZstCallbackQueue<ZstEntityTemplateEventCallback, ZstEntityBase*> * entity_template_arriving_events();
    ZstCallbackQueue<ZstEntityTemplateEventCallback, ZstEntityBase*> * entity_template_leaving_events();
	ZstCallbackQueue<ZstPlugEventCallback, ZstURI> * plug_arriving_events();
	ZstCallbackQueue<ZstPlugEventCallback, ZstURI> * plug_leaving_events();
	ZstCallbackQueue<ZstCableEventCallback, ZstCable> * cable_arriving_events();
	ZstCallbackQueue<ZstCableEventCallback, ZstCable> * cable_leaving_events();
	
	//Debugging
	ZST_EXPORT int graph_recv_tripmeter();
	ZST_EXPORT void reset_graph_recv_tripmeter();
	ZST_EXPORT int graph_send_tripmeter();
	ZST_EXPORT void reset_graph_send_tripmeter();

private:
	//Stage actor
	void start();
	void stop();

	//Registration
	void leave_stage();
    std::string first_available_ext_ip();

	//Internal send and receive
	void send_to_stage(zmsg_t * msg);
	void send_through_stage(zmsg_t * msg);
	zmsg_t * receive_from_stage();
	zmsg_t * receive_stage_update();
	zmsg_t * receive_routed_from_stage();
	inline static ZstMessages::Signal check_stage_response_ok();

	//Socket handlers
	static int s_handle_graph_in(zloop_t *loop, zsock_t *sock, void *arg);
	static int s_handle_stage_update_in(zloop_t *loop, zsock_t *sock, void *arg);
	static int s_handle_stage_router(zloop_t *loop, zsock_t *sock, void *arg);

	//Message handlers
	void stage_update_handler(zsock_t * socket, zmsg_t * msg);
	void connect_performer_handler(zsock_t * socket, zmsg_t * msg);
    void create_entity_from_template_handler(zsock_t * socket, zmsg_t * msg);
	void broadcast_to_local_plugs(const ZstURI & output_plug, const ZstValue & value);

	//Heartbeat timer
	int m_heartbeat_timer_id;
	static int s_heartbeat_timer(zloop_t *loop, int timer_id, void *arg);

	//Destruction
	bool m_is_ending;
	bool m_is_destroyed;
	bool m_connected_to_stage = false;
	ZstReaper * m_reaper;

	//UUIDs
	zuuid_t * m_startup_uuid;
	std::string m_assigned_uuid;

	//Name property
	std::string m_output_endpoint;
    std::string m_network_interface;

	//Entities
    ZstComponent * m_root_performer;
	std::unordered_map<ZstURI, ZstEntityBase* > m_entities;
    std::unordered_map<ZstURI, ZstEntityBase*> m_template_entities;
	std::unordered_map<ZstURI, ZstEntityBase*> & entities();
    Queue<ZstInputPlug*> m_compute_queue;

	//Active local plug connections
	std::vector<ZstCable*> m_local_cables;
	std::vector<ZstCable*> & cables();
	
	//Events and callbacks
	ZstCallbackQueue<ZstEntityEventCallback, ZstEntityBase*> * m_entity_arriving_event_manager;
	ZstCallbackQueue<ZstEntityEventCallback, ZstEntityBase*> * m_entity_leaving_event_manager;
    ZstCallbackQueue<ZstEntityTemplateEventCallback, ZstEntityBase*> * m_entity_template_arriving_event_manager;
    ZstCallbackQueue<ZstEntityTemplateEventCallback, ZstEntityBase*> * m_entity_template_leaving_event_manager;
	ZstCallbackQueue<ZstCableEventCallback, ZstCable> * m_cable_arriving_event_manager;
	ZstCallbackQueue<ZstCableEventCallback, ZstCable> * m_cable_leaving_event_manager;
	ZstCallbackQueue<ZstPlugEventCallback, ZstURI> * m_plug_arriving_event_manager;
	ZstCallbackQueue<ZstPlugEventCallback, ZstURI> * m_plug_leaving_event_manager;

	//Zeromq pipes
	zsock_t *m_stage_requests;		//Reqests sent to the stage server
	zsock_t *m_stage_router;        //Stage pipe in
	zsock_t *m_stage_updates;		//Stage publisher for updates
	zsock_t *m_graph_out;           //Pub for sending graph outputs
	zsock_t *m_graph_in;            //Sub for receiving graph inputs

	//Addresses
	std::string m_stage_addr = "127.0.0.1";
	std::string m_stage_requests_addr;
	std::string m_stage_router_addr;
	std::string m_stage_updates_addr;
	std::string m_graph_out_addr;

	//Debugging
	int m_num_graph_recv_messages;
	int m_num_graph_send_messages;
};
