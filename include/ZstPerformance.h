#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include "ZstExports.h"
#include "czmq.h"
#include "ZstMessages.h"
#include "ZstPlug.h"
#include "ZstActor.h"

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
    
    ZST_EXPORT ZstPlug* create_plug(std::string name, std::string instrument, ZstPlug::Direction direction);
    ZST_EXPORT void destroy_plug(ZstPlug *plug);
    ZST_EXPORT std::vector<ZstPlug::Address> get_all_plug_addresses(std::string section = "", std::string instrument = "");

	ZST_EXPORT void connect_plugs(ZstPlug::Address a, ZstPlug::Address b);

private:
    ZstPerformance(std::string name);

    //Name property
    std::string m_performer_name;

    //All plugs owned by this section
    std::map<std::string, std::vector<ZstPlug*>> m_plugs;
    
    //Default ports
    int m_stage_req_port = 6000;
    int m_stage_pipe_port = -1;
    
    //Stage actor
    void start();
	void stop();
    
    //Zeromq pipes
    zsock_t *m_stage_requests;		//Reqests sent to the stage server
    zsock_t *m_stage_pipe;          //Stage pipe in/out
    zsock_t *m_graph_out;           //Pub for sending graph updates
    zsock_t *m_graph_in;            //Sub for receiving graph updates
    
    //Socket handlers
    static int s_handle_graph_in(zloop_t *loop, zsock_t *sock, void *arg);
    static int s_handle_stage_pipe(zloop_t *loop, zsock_t *sock, void *arg);

    //Heartbeat timer
    static int s_heartbeat_timer(zloop_t *loop, int timer_id, void *arg);
};


