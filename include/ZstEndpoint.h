#pragma once

#include <map>
#include <vector>
#include <chrono>
#include <czmq.h>
#include <string>
#include <msgpack.hpp>
#include "ZstActor.h"
#include "ZstExports.h"

//Fixed port numbers
#define STAGE_REP_PORT 6000
#define STAGE_ROUTER_PORT 6001

//Forward declarations
class ZstURI;
class ZstPerformer;
class ZstPlug;

class ZstEndpoint : public ZstActor {
public:
	ZstEndpoint();
	~ZstEndpoint();
	void init(std::string stage_address);
	ZST_EXPORT void destroy();

	void send_to_graph(zmsg_t * msg);
	zmsg_t * receive_from_graph();

	void register_endpoint_to_stage();
	ZstPerformer * create_performer(std::string name);
	ZstPerformer * get_performer_by_name(std::string performer);
	
	template<typename T>
	ZST_EXPORT static T* create_plug(ZstURI uri);
	ZST_EXPORT void destroy_plug(ZstPlug * plug);

	ZST_EXPORT  std::vector<ZstURI> get_all_plug_addresses(std::string performer="", std::string instrument="");
	void connect_plugs(ZstURI a, ZstURI b);
	
	ZST_EXPORT std::chrono::milliseconds ping_stage();

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
	zmsg_t * receive_routed_from_stage();

	//Socket handlers
	static int s_handle_graph_in(zloop_t *loop, zsock_t *sock, void *arg);
	static int s_handle_stage_router(zloop_t *loop, zsock_t *sock, void *arg);

	//Message handlers
	void connect_performer_handler(zsock_t * socket, zmsg_t * msg);
	void broadcast_to_local_plugs(ZstURI output_plug, msgpack::object obj);

	//Heartbeat timer
	static int s_heartbeat_timer(zloop_t *loop, int timer_id, void *arg);

	//Destruction
	bool m_is_ending;

	zuuid_t * m_startup_uuid;
	std::string m_assigned_uuid;

	//Name property
	std::string m_output_endpoint;
	std::string m_stage_addr = "127.0.0.1";

	//All performers
	std::map<std::string, ZstPerformer*> m_performers;

	//Active local plug connections
	std::map<ZstURI, std::vector<ZstPlug*>> m_plug_connections;

	//Zeromq pipes
	zsock_t *m_stage_requests;		//Reqests sent to the stage server
	zsock_t *m_stage_router;        //Stage pipe in
	zsock_t *m_graph_out;           //Pub for sending graph updates
	zsock_t *m_graph_in;            //Sub for receiving graph updates
};