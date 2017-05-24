#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <boost\uuid\uuid.hpp>
#include <boost\uuid\uuid_generators.hpp>
#include <boost\uuid\uuid_io.hpp>
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
	ZST_EXPORT void destroy();
	//Singleton accessor
	ZST_EXPORT static Showtime & instance();

	//Init
    ZST_EXPORT static void join(std::string stage_address);

    //Stage methods
    ZST_EXPORT std::chrono::milliseconds ping_stage();

	//Performers are our local containers for plugs
	ZST_EXPORT static ZstPerformer* create_performer(std::string name);
	ZST_EXPORT static ZstPerformer * get_performer(std::string performer);

	template<typename T>
    ZST_EXPORT static T* create_plug(std::string performer, std::string name, std::string instrument, PlugDir direction);
    ZST_EXPORT void destroy_plug(ZstPlug *plug);
    ZST_EXPORT std::vector<PlugAddress> get_all_plug_addresses(std::string section = "", std::string instrument = "");
	ZST_EXPORT static void connect_plugs(PlugAddress a, PlugAddress b);
    
	//ZST_EXPORT void fire_plug(ZstPlug *plug, zframe_t * frame);
	void send_to_graph(zmsg_t * msg);
	zmsg_t * receive_from_graph();

private:
    Showtime();
	Showtime(const Showtime&); // Prevent construction by copying
	Showtime& operator=(const Showtime&) {}; // Prevent assignment
	void init(std::string stage_address);

	//Stage actor
	void start();
	void stop();

	void register_performer_to_stage(std::string);
	void send_to_stage(zmsg_t * msg);
	void send_through_stage(zmsg_t * msg);

	zmsg_t * receive_from_stage();
	zmsg_t * receive_routed_from_stage();

	//Socket handlers
	static int s_handle_graph_in(zloop_t *loop, zsock_t *sock, void *arg);
	static int s_handle_stage_router(zloop_t *loop, zsock_t *sock, void *arg);

	//Message handlers
	void connect_performer_handler(zsock_t * socket, zmsg_t * msg);

	//Heartbeat timer
	static int s_heartbeat_timer(zloop_t *loop, int timer_id, void *arg);

	//Destruction
	bool m_is_ending;

    //Name property
    std::string m_output_endpoint;
	std::string m_stage_addr = "127.0.0.1";
	boost::uuids::uuid m_client_uuid;

    //All performers
    std::map<std::string, ZstPerformer*> m_performers;
    
    //Zeromq pipes
    zsock_t *m_stage_requests;		//Reqests sent to the stage server
    zsock_t *m_stage_router;          //Stage pipe in
    zsock_t *m_graph_out;           //Pub for sending graph updates
    zsock_t *m_graph_in;            //Sub for receiving graph updates
};


