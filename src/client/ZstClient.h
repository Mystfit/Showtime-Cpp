#pragma once

#include <unordered_map>
#include <czmq.h>
#include <string>

//Showtime API includes
#include <ZstCore.h>

//Showtime Core includes
#include "../core/ZstActor.h"
#include "../core/ZstMessage.h"
#include "../core/ZstMessagePool.h"
#include "../core/ZstINetworkInteractor.h"
#include "../core/ZstValue.h"
#include "../core/ZstEventDispatcher.h"

//Client includes
#include "ZstClientEvents.h"

class ZstClient : public ZstActor, public ZstINetworkInteractor {
	friend class ZstCableLeavingEvent;
	friend class ZstComponentLeavingEvent;

public:
	ZstClient();
	~ZstClient();
	void init(const char * client_name, bool debug);
	void init_file_logging(const char * log_file_path);
	void destroy() override;
	void process_callbacks();
	
	//CLient singleton - should not be accessable outside this interface
	static ZstClient & instance();

	//Register this endpoint to the stage
	void register_client_to_stage(std::string stage_address, bool async = false);
    void synchronise_graph(bool async = false);
	void leave_stage(bool immediately = false);
    
	//Stage connection status
	bool is_connected_to_stage();
	long ping();
	
    //Entities
	ZstEntityBase * find_entity(const ZstURI & path);
	ZstPlug * find_plug(const ZstURI & path);
	void activate_entity(ZstEntityBase* entity, bool async = false);
	void destroy_entity(ZstEntityBase * entity, bool async = false);
	bool entity_is_local(ZstEntityBase & entity);
	bool path_is_local(const ZstURI & path);
	void add_proxy_entity(ZstEntityBase & entity);

	//Performers
	ZstPerformer * get_performer_by_URI(const ZstURI & uri) const;
	ZstPerformer * get_local_performer() const;

	//Plugs
	void destroy_plug(ZstPlug * plug);

	//Graph communication
	virtual void publish(ZstPlug * plug) override;

	//Cables
	ZstCable * connect_cable(ZstPlug * input, ZstPlug * output, bool async = false);
	void destroy_cable(ZstCable * cable, bool async = false);
	void disconnect_plug(ZstPlug * plug);
	void disconnect_plugs(ZstPlug * input_plug, ZstPlug * output_plug);
	
	//Callbacks
	ZstEventDispatcher * client_connected_events();
	ZstEventDispatcher * client_disconnected_events();
	ZstEventDispatcher * performer_arriving_events();
	ZstEventDispatcher * performer_leaving_events();
	ZstEventDispatcher * component_arriving_events();
	ZstEventDispatcher * component_leaving_events();
	ZstEventDispatcher * component_type_arriving_events();
	ZstEventDispatcher * component_type_leaving_events();
	ZstEventDispatcher * plug_arriving_events();
	ZstEventDispatcher * plug_leaving_events();
	ZstEventDispatcher * cable_arriving_events();
	ZstEventDispatcher * cable_leaving_events();
	
	//Debugging
	int graph_recv_tripmeter();
	void reset_graph_recv_tripmeter();
	int graph_send_tripmeter();
	void reset_graph_send_tripmeter();

	//Network interactor implementation
	virtual void enqueue_synchronisable_event(ZstSynchronisable * synchronisable) override;

private:
	//Stage actor
	void start() override;
	void stop() override;

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

	//Performers
    ZstPerformer * m_root;
	ZstPerformerMap m_clients;
	void add_performer(ZstPerformer & performer);

	//Event hooks
	void flush_events();
	ZstSynchronisableDeferredEvent * m_synchronisable_deferred_event;
	ZstComponentLeavingEvent * m_performer_leaving_hook;
	ZstComponentLeavingEvent * m_component_leaving_hook;
	ZstCableLeavingEvent * m_cable_leaving_hook;
	ZstPlugLeavingEvent * m_plug_leaving_hook;
	ZstComputeEvent * m_compute_event;

	//Stage communication
	void register_client_to_stage_sync(MessageFuture & future);
	void register_client_to_stage_async(MessageFuture & future);
	void register_client_complete(ZstMsgKind status);
	void synchronise_graph_sync(MessageFuture & future);
	void synchronise_graph_async(MessageFuture & future);
	void synchronise_graph_complete(ZstMsgKind status);
	void leave_stage_complete();
	void activate_entity_sync(ZstEntityBase * entity, MessageFuture & future);
	void activate_entity_async(ZstEntityBase * entity, MessageFuture & future);
	void activate_entity_complete(ZstMsgKind status, ZstEntityBase * entity);
	void destroy_entity_sync(ZstEntityBase * entity, MessageFuture & future);
	void destroy_entity_async(ZstEntityBase * entity, MessageFuture & future);
	void destroy_entity_complete(ZstMsgKind status, ZstEntityBase * entity);
	void connect_cable_sync(ZstCable * cable, MessageFuture & future);
	void connect_cable_async(ZstCable * cable, MessageFuture & future);
	void connect_cable_complete(ZstMsgKind status, ZstCable * cable);
	void destroy_cable_sync(ZstCable * cable, MessageFuture & future);
	void destroy_cable_async(ZstCable * cable, MessageFuture & future);
	void destroy_cable_complete(ZstMsgKind status, ZstCable * cable);
	void destroy_plug_complete(int status);
	
	//Cable storage
	ZstCable * create_cable_ptr(ZstCable & cable);
	ZstCable * create_cable_ptr(ZstPlug * output, ZstPlug * input);
	ZstCable * create_cable_ptr(const ZstURI & input_path, const ZstURI & output_path);
	ZstCable * find_cable_ptr(const ZstURI & input_path, const ZstURI & output_path);
	ZstCable * find_cable_ptr(ZstPlug * input, ZstPlug * output);
	ZstCableList m_cables;
	
	//Events and callbacks
	ZstEventDispatcher * m_client_connected_event_manager;
	ZstEventDispatcher * m_client_disconnected_event_manager;
	ZstEventDispatcher * m_performer_arriving_event_manager;
	ZstEventDispatcher * m_performer_leaving_event_manager;
	ZstEventDispatcher * m_component_arriving_event_manager;
	ZstEventDispatcher * m_component_leaving_event_manager;
	ZstEventDispatcher * m_component_type_arriving_event_manager;
	ZstEventDispatcher * m_component_type_leaving_event_manager;
	ZstEventDispatcher * m_cable_arriving_event_manager;
	ZstEventDispatcher * m_cable_leaving_event_manager;
	ZstEventDispatcher * m_plug_arriving_event_manager;
	ZstEventDispatcher * m_plug_leaving_event_manager;
	ZstEventDispatcher * m_compute_event_manager;
	ZstEventDispatcher * m_synchronisable_event_manager;
		
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
