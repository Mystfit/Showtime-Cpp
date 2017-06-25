#pragma once

#include <map>
#include <vector>
#include <chrono>
#include <czmq.h>
#include <string>
#include <msgpack.hpp>
#include "ZstActor.h"
#include "ZstExports.h"
#include "Queue.h"
#include "ZstPlug.h"
#include "ZstEvent.h"

//Fixed port numbers
#define STAGE_REP_PORT 6000
#define STAGE_ROUTER_PORT 6001
#define STAGE_PUB_PORT 6002

//Forward declarations
class ZstURI;
class ZstPerformer;

class ZstEndpoint : public ZstActor {
public:
	ZstEndpoint();
	~ZstEndpoint();
	ZST_EXPORT void init();
	ZST_EXPORT void destroy();

	void send_to_graph(zmsg_t * msg);
	zmsg_t * receive_from_graph();

	//Register this endpoint to the stage
	void register_endpoint_to_stage(std::string stage_address);

	//Stage connection status
	bool is_connected_to_stage();

	//Lets the stage know we want a full snapshot of the current performance
	void request_stage_sync();

	ZstPerformer * create_performer(std::string name);
	ZstPerformer * get_performer_by_URI(std::string uri_str);
	
	template<typename T>
	ZST_EXPORT static T* create_plug(ZstURI * uri);
	ZST_EXPORT static ZstIntPlug * create_int_plug(ZstURI * uri);

	ZST_EXPORT void destroy_plug(ZstPlug * plug);

	void connect_plugs(const ZstURI * a, const ZstURI * b);
	
	ZST_EXPORT std::chrono::milliseconds ping_stage();

	void enqueue_plug_event(ZstEvent event);
	ZST_EXPORT ZstEvent pop_plug_event();
	ZST_EXPORT int plug_event_queue_size();

private:
	//Stage actor
	void start();
	void stop();

	//Registration
	void register_performer_to_stage(std::string);

	//Send and receive
	void send_to_stage(zmsg_t * msg);
	void send_through_stage(zmsg_t * msg);
	zmsg_t * receive_from_stage();
	zmsg_t * receive_stage_update();
	zmsg_t * receive_routed_from_stage();

	//Socket handlers
	static int s_handle_graph_in(zloop_t *loop, zsock_t *sock, void *arg);
	static int s_handle_stage_update_in(zloop_t *loop, zsock_t *sock, void *arg);
	static int s_handle_stage_router(zloop_t *loop, zsock_t *sock, void *arg);

	//Message handlers
	void stage_update_handler(zsock_t * socket, zmsg_t * msg);
	void connect_performer_handler(zsock_t * socket, zmsg_t * msg);
	void broadcast_to_local_plugs(ZstURI output_plug, msgpack::object obj);

	//Heartbeat timer
	static int s_heartbeat_timer(zloop_t *loop, int timer_id, void *arg);

	//Destruction
	bool m_is_ending;
	bool m_connected_to_stage = false;

	zuuid_t * m_startup_uuid;
	std::string m_assigned_uuid;

	//Name property
	std::string m_output_endpoint;
	std::string m_stage_addr = "127.0.0.1";
    std::string m_network_interface;

	//All performers
	std::map<std::string, ZstPerformer*> m_performers;

	//Active local plug connections
	std::map<ZstURI, std::vector<ZstPlug*>> m_plug_connections;

	Queue<ZstEvent> m_plug_events;

	//Zeromq pipes
	zsock_t *m_stage_requests;		//Reqests sent to the stage server
	zsock_t *m_stage_router;        //Stage pipe in
	zsock_t *m_stage_updates;
	zsock_t *m_graph_out;           //Pub for sending graph outputs
	zsock_t *m_graph_in;            //Sub for receiving graph inputs
};
