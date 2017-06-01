#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include "czmq.h"
#include "ZstExports.h"
#include "ZstPlug.h"
#include "ZstPerformer.h"
#include "ZstMessages.h"
#include "ZstActor.h"

#define STAGE_REP_PORT 6000
#define STAGE_ROUTER_PORT 6001

class ZstPlug;

class Showtime : public ZstActor
{
public:
	ZST_EXPORT ~Showtime();
	ZST_EXPORT static void destroy();
	ZST_EXPORT void destroy_singleton();
	//Singleton accessor
	ZST_EXPORT static Showtime & instance();

	//Init
    ZST_EXPORT static void join(std::string stage_address);

    //Stage methods
    ZST_EXPORT std::chrono::milliseconds ping_stage();

	//Performers are our local containers for plugs
	ZST_EXPORT static ZstPerformer* create_performer(std::string name);
	ZST_EXPORT static ZstPerformer * get_performer_by_name(std::string performer);

	template<typename T>
    ZST_EXPORT static T* create_plug(ZstURI uri);
    ZST_EXPORT void destroy_plug(ZstPlug *plug);
    ZST_EXPORT std::vector<ZstURI> get_all_plug_addresses(std::string section = "", std::string instrument = "");
	ZST_EXPORT static void connect_plugs(ZstURI a, ZstURI b);
    
	//ZST_EXPORT void fire_plug(ZstPlug *plug, zframe_t * frame);
	ZST_EXPORT void send_to_graph(zmsg_t * msg);
	ZST_EXPORT zmsg_t * receive_from_graph();

private:
    Showtime();
	Showtime(const Showtime&); // Prevent construction by copying
	Showtime& operator=(const Showtime&) {}; // Prevent assignment
	void init(std::string stage_address);

	//Stage actor
	void start();
	void stop();
    
    //Registration
	void register_endpoint_to_stage();
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
    zsock_t *m_stage_router;          //Stage pipe in
    zsock_t *m_graph_out;           //Pub for sending graph updates
    zsock_t *m_graph_in;            //Sub for receiving graph updates
};


