#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include "ZstExports.h"
#include "czmq.h"
#include "ZstMessages.h"
#include "ZstActor.h"
#include "ZstPlug.h"

#define STAGE_REP_PORT 6000
#define STAGE_ROUTER_PORT 6001

class ZstPerformance : public ZstActor
{
public:
    //Factory
    ZST_EXPORT static ZstPerformance* create_performer(std::string performer_name);
    ZST_EXPORT ~ZstPerformance();
    
    //Accessors
    ZST_EXPORT std::string get_performer_name();
    
    //Lists
    ZST_EXPORT std::vector<ZstPlug*> get_all_plugs();
    ZST_EXPORT std::vector<ZstPlug*> get_instrument_plugs(std::string instrument);

    //Stage methods
    ZST_EXPORT void register_to_stage();
    ZST_EXPORT std::chrono::milliseconds ping_stage();

    ZST_EXPORT ZstPlug* create_plug(std::string name, std::string instrument, PlugDirection direction);
    ZST_EXPORT void destroy_plug(ZstPlug *plug);
    ZST_EXPORT std::vector<PlugAddress> get_all_plug_addresses(std::string section = "", std::string instrument = "");

	ZST_EXPORT void connect_plugs(PlugAddress a, PlugAddress b);
    
    template<typename T>
    void fire_plug(ZstPlug *plug, T data)
    {
        send_to_graph(ZstMessages::build_graph_message<T>(plug->get_address(), data));
    }
    //Plug primitive types
    ZST_EXPORT template void fire_plug(ZstPlug *plug, IntOutput);
    //ZST_EXPORT template void fire_plug(ZstPlug *plug, ListIntOutput);
    ZST_EXPORT template void fire_plug(ZstPlug *plug, FloatOutput);
    //ZST_EXPORT template void fire_plug(ZstPlug *plug, ListFloatOutput);
    ZST_EXPORT template void fire_plug(ZstPlug *plug, StringOutput);

private:
    ZstPerformance(std::string name);

    //Name property
    std::string m_performer_name;
    std::string m_output_endpoint;

    //All plugs owned by this section
    std::map<std::string, std::vector<ZstPlug*>> m_plugs;
    
    //Stage actor
    void start();
	void stop();
    
    //Zeromq pipes
    zsock_t *m_stage_requests;		//Reqests sent to the stage server
    zsock_t *m_stage_router;          //Stage pipe in
    zsock_t *m_graph_out;           //Pub for sending graph updates
    zsock_t *m_graph_in;            //Sub for receiving graph updates
    
    void send_to_stage(zmsg_t * msg);
    void send_through_stage(zmsg_t * msg);
    void send_to_graph(zmsg_t * msg);
    zmsg_t * receive_from_stage();
    zmsg_t * receive_routed_from_stage();
    zmsg_t * receive_from_graph();


    //Socket handlers
    static int s_handle_graph_in(zloop_t *loop, zsock_t *sock, void *arg);
    static int s_handle_stage_router(zloop_t *loop, zsock_t *sock, void *arg);
    
    //Message handlers
    void connect_performer_handler(zsock_t * socket, zmsg_t * msg);

    //Heartbeat timer
    static int s_heartbeat_timer(zloop_t *loop, int timer_id, void *arg);
    

};


