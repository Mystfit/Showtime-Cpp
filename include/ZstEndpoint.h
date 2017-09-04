#pragma once

#include <map>
#include <vector>
#include <czmq.h>
#include <string>
#include <msgpack.hpp>
#include "ZstActor.h"
#include "ZstExports.h"
#include "Queue.h"
#include "Showtime.h"
#include "ZstPlug.h"
#include "ZstEvent.h"
#include "ZstMessages.h"
#include "ZstCable.h"
#include "ZstCallbackQueue.h"

//Forward declarations
class ZstValue;
class ZstURI;
class ZstPerformer;
class ZstEntityBase;
class ZstComponent;

class ZstEndpoint : public ZstActor {
public:
	friend class Showtime;
	ZstEndpoint();
	~ZstEndpoint();
	ZST_EXPORT void init();
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

	//Entities
	void register_entity_type(const char * entity_type);
	int register_entity(ZstEntityBase * entity);
	int destroy_entity(ZstEntityBase * entity);
	ZstEntityBase * get_entity_by_URI(ZstURI uri);
	
	//Plugs
	template<typename T>
	ZST_EXPORT static T* create_plug(ZstComponent * owner, const char * name, ZstValueType val_type, PlugDirection direction);
	ZST_EXPORT int destroy_plug(ZstPlug * plug);

	//Cables
	int connect_cable(ZstURI a, ZstURI b);
	int destroy_cable(ZstURI a, ZstURI b);
	ZST_EXPORT std::vector<ZstCable*> get_cables_by_URI(const ZstURI & uri);
	ZST_EXPORT ZstCable * get_cable_by_URI(const ZstURI & uriA, const ZstURI & uriB);
	void remove_cable(ZstCable * cable);
	
	ZST_EXPORT int ping_stage();

	void enqueue_event(ZstEvent * event);
	ZST_EXPORT ZstEvent * pop_event();
	ZST_EXPORT size_t event_queue_size();

	//Plug callbacks
	ZST_EXPORT ZstCallbackQueue<ZstEntityEventCallback, ZstURI> * entity_arriving_events();
	ZST_EXPORT ZstCallbackQueue<ZstEntityEventCallback, ZstURI> * entity_leaving_events();
	ZST_EXPORT ZstCallbackQueue<ZstPlugEventCallback, ZstURI> * plug_arriving_events();
	ZST_EXPORT ZstCallbackQueue<ZstPlugEventCallback, ZstURI> * plug_leaving_events();
	ZST_EXPORT ZstCallbackQueue<ZstCableEventCallback, ZstCable> * cable_arriving_events();
	ZST_EXPORT ZstCallbackQueue<ZstCableEventCallback, ZstCable> * cable_leaving_events();

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
	void broadcast_to_local_plugs(ZstURI output_plug, ZstValue & value);

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

	//Name property
	std::string m_output_endpoint;
    std::string m_network_interface;

	//All performers
	std::map<ZstURI, ZstEntityBase*> m_entities;
	std::map<ZstURI, ZstEntityBase*> & entities();

	//Active local plug connections
	std::vector<ZstCable*> m_local_cables;
	std::vector<ZstCable*> & cables();
	
	//Events and callbacks
	Queue<ZstEvent*> m_events;
	ZstCallbackQueue<ZstEntityEventCallback, ZstURI> * m_performer_arriving_event_manager;
	ZstCallbackQueue<ZstEntityEventCallback, ZstURI> * m_performer_leaving_event_manager;
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
	Str255 m_stage_requests_addr;
	Str255 m_stage_router_addr;
	Str255 m_stage_updates_addr;
	Str255 m_graph_out_addr;
};
