#pragma once

#include <unordered_map>
#include <vector>
#include <czmq.h>
#include <string>
#include <msgpack.hpp>

#include <ZstURI.h>
#include <ZstExports.h>
#include <ZstCallbacks.h>

//Core headers
#include "../core/Queue.h"
#include "../core/ZstActor.h"
#include "../core/ZstMessage.h"
#include "../core/ZstGraphSender.h"
#include "../core/ZstCallbackQueue.h"
#include "../core/ZstMessagePool.h"

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
class ZstPerformer;

#define MESSAGE_POOL_BLOCK 256

class ZstClient : public ZstActor, public ZstGraphSender {
public:
	friend class Showtime;
	ZstClient();
	~ZstClient();
	void init(const char * performer_name);
	void destroy();
	void process_callbacks();

	//CLient singleton - should not be accessable outside this interface
	static ZstClient & instance();

	//Register this endpoint to the stage
	void register_client_to_stage(std::string stage_address);
	void register_client_complete(int status);
	void leave_stage();
	
	//Stage connection status
	bool is_connected_to_stage();
	long ping();
    
	//Entity type registration
	int register_component_type(ZstComponent * entity);
    int unregister_component_type(ZstComponent * entity);
    int run_component_template(ZstComponent * entity);
    
    //Entities
	ZstEntityBase * find_entity(const ZstURI & path);
	ZstPlug * find_plug(const ZstURI & path);
	void activate_entity(ZstEntityBase* entity);
	void destroy_entity(ZstEntityBase * entity);
	void destroy_entity_completed(int status);

	bool entity_is_local(ZstEntityBase * entity);
	bool path_is_local(const ZstURI & path);
	void add_proxy_entity(ZstEntityBase * entity);

	//Performers
	std::unordered_map<ZstURI, ZstPerformer*> & performers();
	void add_performer(ZstPerformer * performer);
	ZstPerformer * get_performer_by_URI(const ZstURI & uri) const;
	ZstPerformer * get_local_performer() const;

	//Plugs
	void destroy_plug(ZstPlug * plug);
	void destroy_plug_complete(int status);
    void enqueue_compute(ZstInputPlug * plug);

	//Graph communication
	virtual void publish(ZstPlug * plug) override;

	//Cables
	ZstCable * connect_cable(ZstPlug * a, ZstPlug * b);
	void connect_cable_completed(int status);
	void destroy_cable(ZstCable * cable);
	void destroy_cable_completed(int status);
	void disconnect_plug(ZstPlug * plug);
	void disconnect_plugs(ZstPlug * input_plug, ZstPlug * output_plug);
	
	//Callbacks
	ZstCallbackQueue<ZstClientConnectionEvent, ZstPerformer*> * client_connected_events();
	ZstCallbackQueue<ZstPerformerEvent, ZstPerformer*> * performer_arriving_events();
	ZstCallbackQueue<ZstPerformerEvent, ZstPerformer*> * performer_leaving_events();
	ZstCallbackQueue<ZstComponentEvent, ZstComponent*> * component_arriving_events();
	ZstCallbackQueue<ZstComponentEvent, ZstComponent*> * component_leaving_events();
	ZstCallbackQueue<ZstEntityEvent, ZstEntityBase*> * entity_activated_events();
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
    std::string first_available_ext_ip();

	//Internal send and receive
	//Send/receive
	void send_to_stage(ZstMessage * msg);
	ZstMessage * receive_from_stage();
	ZstMessage * receive_stage_update();

	//Message pools
	ZstMessagePool * msg_pool();
	ZstMessagePool * m_message_pool;
	
	//Socket handlers
	static int s_handle_graph_in(zloop_t *loop, zsock_t *sock, void *arg);
	static int s_handle_stage_update_in(zloop_t *loop, zsock_t *sock, void *arg);
	static int s_handle_stage_router(zloop_t *loop, zsock_t *sock, void *arg);

	//Message handlers
	void stage_update_handler(ZstMessage * msg);
	void connect_client_handler(const char * endpoint_ip, const char * output_plug);
    void create_entity_from_template_handler(const ZstURI & entity_template_address);

	//Heartbeat timer
	int m_heartbeat_timer_id;
	long m_ping;
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
	std::string m_graph_out_ip;
    std::string m_network_interface;

	//Root performer
    ZstPerformer * m_root;
	std::unordered_map<ZstURI, ZstPerformer* > m_clients;
    Queue<ZstInputPlug*> m_compute_queue;

	//Callback hooks for leaving items
	static void component_leaving_hook(void * target);
	static void component_type_leaving_hook(void * target);
	static void plug_leaving_hook(void * target);
	static void cable_leaving_hook(void * target);
	
	//Cable storage
	ZstCable * create_cable_ptr(ZstCable & cable);
	void remove_cable(ZstCable * cable);
	ZstCable * find_cable_ptr(const ZstURI & input_path, const ZstURI & output_path);
	
	//Events and callbacks
	ZstCallbackQueue<ZstClientConnectionEvent, ZstPerformer*> * m_client_connected_event_manager;
	ZstCallbackQueue<ZstPerformerEvent, ZstPerformer*> * m_performer_arriving_event_manager;
	ZstCallbackQueue<ZstPerformerEvent, ZstPerformer*> * m_performer_leaving_event_manager;
	ZstCallbackQueue<ZstEntityEvent, ZstEntityBase*> * m_entity_activated_event_manager;
	ZstCallbackQueue<ZstComponentEvent, ZstComponent*> * m_component_arriving_event_manager;
	ZstCallbackQueue<ZstComponentEvent, ZstComponent*> * m_component_leaving_event_manager;
    ZstCallbackQueue<ZstComponentTypeEvent, ZstEntityBase*> * m_component_type_arriving_event_manager;
    ZstCallbackQueue<ZstComponentTypeEvent, ZstEntityBase*> * m_component_type_leaving_event_manager;
	ZstCallbackQueue<ZstCableEvent, ZstCable*> * m_cable_arriving_event_manager;
	ZstCallbackQueue<ZstCableEvent, ZstCable*> * m_cable_leaving_event_manager;
	ZstCallbackQueue<ZstPlugEvent, ZstPlug*> * m_plug_arriving_event_manager;
	ZstCallbackQueue<ZstPlugEvent, ZstPlug*> * m_plug_leaving_event_manager;

	//Zeromq pipes
	zsock_t *m_stage_router;        //Stage pipe in
	zsock_t *m_stage_updates;		//Stage publisher for updates
	zsock_t *m_graph_out;           //Pub for sending graph outputs
	zsock_t *m_graph_in;            //Sub for receiving graph inputs

	//Addresses
	std::string m_stage_addr = "127.0.0.1";
	std::string m_stage_router_addr;
	std::string m_stage_updates_addr;
	std::string m_graph_out_addr;

	//Debugging
	int m_num_graph_recv_messages;
	int m_num_graph_send_messages;
};
