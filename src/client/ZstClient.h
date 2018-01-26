#pragma once

#include <unordered_map>
#include <vector>
#include <czmq.h>
#include <string>
#include <msgpack.hpp>

//Showtime API includes
#include <ZstConstants.h>
#include <ZstURI.h>
#include <ZstExports.h>
#include <ZstEvents.h>

//Showtime Core includes
#include "../core/Queue.h"
#include "../core/ZstActor.h"
#include "../core/ZstMessage.h"
#include "../core/ZstINetworkInteractor.h"
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

typedef ZstCallbackQueue<ZstPerformerEvent, ZstPerformer*> ZstPerformerEventQueue;
typedef ZstCallbackQueue<ZstComponentEvent, ZstComponent*> ZstComponentEventQueue;
typedef ZstCallbackQueue<ZstComponentTypeEvent, ZstComponent*> ZstComponentTypeEventQueue;
typedef ZstCallbackQueue<ZstPlugEvent, ZstPlug*> ZstPlugEventQueue;
typedef ZstCallbackQueue<ZstCableEvent, ZstCable*> ZstCableEventQueue;

class ZstClient : public ZstActor, public ZstINetworkInteractor {
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
	void register_client_complete(ZstMessage::Kind status);
	void leave_stage();
	
	//Stage connection status
	bool is_connected_to_stage();
	long ping();
	
    //Entities
	ZstEntityBase * find_entity(const ZstURI & path);
	ZstPlug * find_plug(const ZstURI & path);
	void activate_entity(ZstEntityBase* entity);
	void destroy_entity(ZstEntityBase * entity);
	void destroy_entity_complete(ZstEntityBase * entity);
	bool entity_is_local(ZstEntityBase & entity);
	bool path_is_local(const ZstURI & path);
	void add_proxy_entity(ZstEntityBase & entity);

	//Performers
	std::unordered_map<ZstURI, ZstPerformer*> & performers();
	void add_performer(ZstPerformer & performer);
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
	void destroy_cable(ZstCable * cable);
	void disconnect_plug(ZstPlug * plug);
	void disconnect_plugs(ZstPlug * input_plug, ZstPlug * output_plug);
	
	//Callbacks
	ZstPerformerEventQueue * client_connected_events();
	ZstPerformerEventQueue * performer_arriving_events();
	ZstPerformerEventQueue * performer_leaving_events();
	ZstComponentEventQueue * component_arriving_events();
	ZstComponentEventQueue * component_leaving_events();
    ZstComponentTypeEventQueue * component_type_arriving_events();
    ZstComponentTypeEventQueue * component_type_leaving_events();
	ZstPlugEventQueue * plug_arriving_events();
	ZstPlugEventQueue * plug_leaving_events();
	ZstCableEventQueue * cable_arriving_events();
	ZstCableEventQueue * cable_leaving_events();
	
	//Debugging
	int graph_recv_tripmeter();
	void reset_graph_recv_tripmeter();
	int graph_send_tripmeter();
	void reset_graph_send_tripmeter();

	//Network interactor implementation
	virtual void queue_synchronisable_activation(ZstSynchronisable * synchronisable);
	virtual void queue_synchronisable_deactivation(ZstSynchronisable * synchronisable);

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
	ZstCable * create_cable_ptr(ZstPlug * output, ZstPlug * input);

	void remove_cable(ZstCable * cable);
	ZstCable * find_cable_ptr(const ZstURI & input_path, const ZstURI & output_path);
	ZstCable * find_cable_ptr(ZstPlug * input, ZstPlug * output);
	
	//Events and callbacks
	ZstCallbackQueue<ZstPerformerEvent, ZstPerformer*> * m_client_connected_event_manager;
	ZstCallbackQueue<ZstPerformerEvent, ZstPerformer*> * m_performer_arriving_event_manager;
	ZstCallbackQueue<ZstPerformerEvent, ZstPerformer*> * m_performer_leaving_event_manager;
	ZstComponentEventQueue * m_component_arriving_event_manager;
	ZstComponentEventQueue * m_component_leaving_event_manager;
    ZstComponentTypeEventQueue * m_component_type_arriving_event_manager;
    ZstComponentTypeEventQueue * m_component_type_leaving_event_manager;
	ZstCableEventQueue * m_cable_arriving_event_manager;
	ZstCableEventQueue * m_cable_leaving_event_manager;
	ZstPlugEventQueue * m_plug_arriving_event_manager;
	ZstPlugEventQueue * m_plug_leaving_event_manager;

	//Entities awaiting callback processing
	Queue<ZstSynchronisable*> m_synchronisable_events;

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
