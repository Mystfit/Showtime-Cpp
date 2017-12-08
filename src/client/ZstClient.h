#pragma once

#include <unordered_map>
#include <vector>
#include <czmq.h>
#include <string>
#include <msgpack.hpp>
#include "ZstURI.h"
#include "ZstActor.h"
#include "ZstExports.h"
#include "Queue.h"
#include "ZstMessages.h"
#include "ZstGraphSender.h"
#include "ZstCallbacks.h"
#include "ZstCallbackQueue.h"

//Forward declarations
class ZstValue;
class ZstCable;
class ZstPerformer;
class ZstEntityBase;
class ZstComponent;
class ZstComponentProxy;
class ZstContainer;
class ZstPlug; 
class ZstInputPlug;
class ZstOutputPlug;
class ZstValue;

class ZstClient : public ZstActor, public ZstGraphSender {
public:
	friend class Showtime;
	ZstClient();
	~ZstClient();
	void init(const char * performer_name);
	void destroy();
	void process_callbacks();

	//Endpoint singleton - should not be accessable outside this interface
	static ZstClient & instance();

	//Register this endpoint to the stage
	void register_endpoint_to_stage(std::string stage_address);
	const char * get_endpoint_UUID() const;

	//Stage connection status
	bool is_connected_to_stage();

	//Lets the stage know we want a full snapshot of the current performance
	void signal_sync();
    
	//Entity type registration
	int register_component_type(ZstComponent * entity);
    int unregister_component_type(ZstComponent * entity);
    int run_component_template(ZstComponent * entity);
    
    //Entities
	std::unordered_map<ZstURI, ZstEntityBase*> & entities();
	int activate_entity(ZstEntityBase* entity);
	int destroy_entity(ZstEntityBase * entity);
	bool entity_is_local(ZstEntityBase * entity);
	ZstEntityBase * get_entity_by_URI(const ZstURI & uri) const;
	ZstPlug * get_plug_by_URI(const ZstURI & uri) const;
    ZstContainer * get_root() const;
	template<typename T>
	void create_proxy_entity(const char * buffer, size_t length);

	//Plugs
	int destroy_plug(ZstPlug * plug);
    void enqueue_compute(ZstInputPlug * plug);

	//Graph communication
	virtual void send_to_graph(ZstPlug * plug) override;

	//Cables
	int connect_cable(const ZstURI & a, const ZstURI & b);
	int destroy_cable(ZstCable * cable);
	std::vector<ZstCable*> get_cables_by_URI(const ZstURI & uri);
	ZstCable * get_cable_by_URI(const ZstURI & uriA, const ZstURI & uriB);
	void remove_cable(ZstCable * cable);
	std::vector<ZstCable*> & cables();

    //Utility
	int ping_stage();

	//Plug callbacks
	ZstCallbackQueue<ZstComponentEvent, ZstEntityBase*> * component_arriving_events();
	ZstCallbackQueue<ZstComponentEvent, ZstEntityBase*> * component_leaving_events();
    ZstCallbackQueue<ZstComponentTypeEvent, ZstEntityBase*> * component_type_arriving_events();
    ZstCallbackQueue<ZstComponentTypeEvent, ZstEntityBase*> * component_type_leaving_events();
	ZstCallbackQueue<ZstPlugEvent, ZstPlug*> * plug_arriving_events();
	ZstCallbackQueue<ZstPlugEvent, ZstPlug*> * plug_leaving_events();
	ZstCallbackQueue<ZstCableEvent, ZstCable*> * cable_arriving_events();
	ZstCallbackQueue<ZstCableEvent, ZstCable*> * cable_leaving_events();
	
	//Debugging
	int graph_recv_tripmeter();
	void reset_graph_recv_tripmeter();
	int graph_send_tripmeter();
	void reset_graph_send_tripmeter();

private:
	//Stage actor
	void start();
	void stop();

	//Registration
	void leave_stage();
    std::string first_available_ext_ip();

	//Internal send and receive
	//Send/receive
	void send_to_stage(zmsg_t * msg);
	void send_returnable_to_stage(zmsg_t * msg);
	zmsg_t * receive_from_graph();
	zmsg_t * receive_from_stage();
	zmsg_t * receive_stage_update();
	zmsg_t * receive_routed_from_stage();
	ZstMessages::Signal check_stage_response_ok();

	//Socket handlers
	static int s_handle_graph_in(zloop_t *loop, zsock_t *sock, void *arg);
	static int s_handle_stage_update_in(zloop_t *loop, zsock_t *sock, void *arg);
	static int s_handle_stage_router(zloop_t *loop, zsock_t *sock, void *arg);

	//Message handlers
	void stage_update_handler(zmsg_t * msg);
	void connect_client_handler(const char * endpoint_ip, const char * output_plug);
    void create_entity_from_template_handler(const ZstURI & entity_template_address);

	//Heartbeat timer
	int m_heartbeat_timer_id;
	static int s_heartbeat_timer(zloop_t *loop, int timer_id, void *arg);

	//Destruction
	bool m_is_ending;
	bool m_is_destroyed;
	bool m_connected_to_stage = false;

	//UUIDs
	zuuid_t * m_startup_uuid;
	std::string m_assigned_uuid;
	std::string m_client_name;

	//Name property
	std::string m_output_endpoint;
    std::string m_network_interface;

	//Entities
    ZstContainer * m_root_container;
	std::unordered_map<ZstURI, ZstEntityBase* > m_entities;
    std::unordered_map<ZstURI, ZstEntityBase*> m_template_entities;
    Queue<ZstInputPlug*> m_compute_queue;

	//Callback hooks for leaving items
	static void component_leaving_hook(void * target);
	static void component_type_leaving_hook(void * target);
	static void plug_leaving_hook(void * target);
	static void cable_leaving_hook(void * target);

	//Cable storage
	std::vector<ZstCable*> m_cables;
	
	//Events and callbacks
	ZstCallbackQueue<ZstComponentEvent, ZstEntityBase*> * m_component_arriving_event_manager;
	ZstCallbackQueue<ZstComponentEvent, ZstEntityBase*> * m_component_leaving_event_manager;
    ZstCallbackQueue<ZstComponentTypeEvent, ZstEntityBase*> * m_component_type_arriving_event_manager;
    ZstCallbackQueue<ZstComponentTypeEvent, ZstEntityBase*> * m_component_type_leaving_event_manager;
	ZstCallbackQueue<ZstCableEvent, ZstCable*> * m_cable_arriving_event_manager;
	ZstCallbackQueue<ZstCableEvent, ZstCable*> * m_cable_leaving_event_manager;
	ZstCallbackQueue<ZstPlugEvent, ZstPlug*> * m_plug_arriving_event_manager;
	ZstCallbackQueue<ZstPlugEvent, ZstPlug*> * m_plug_leaving_event_manager;

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
