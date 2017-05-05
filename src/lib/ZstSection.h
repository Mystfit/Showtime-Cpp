#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include "ZstExports.h"
#include "czmq.h"
#include "ZstInstrument.h"
#include "ZstMessages.hpp"

class ZstSection
{
public:
    //Factory
    ZST_EXPORT static ZstSection* create_section(std::string name);
    ZST_EXPORT ~ZstSection();

    // Creates a new instrument
    ZST_EXPORT ZstInstrument* create_instrument(std::string name);

    // Removes and destroys an instrument
    ZST_EXPORT void destroy_instrument(ZstInstrument& instrument);

    //List of all instruments owned by this section
    ZST_EXPORT std::vector<ZstInstrument*>& get_instruments();
    
    ZST_EXPORT void register_to_stage();
    
    ZST_EXPORT std::chrono::milliseconds ping_stage();

private:
    ZstSection(std::string name);

    //Name property
    std::string m_name;

    //All instruments owned by this section
    std::vector<ZstInstrument*> m_instruments;
    
    //Default ports
    int m_stage_req_port = 6000;
    int m_stage_pipe_port = -1;
    
    //Stage actor
    zloop_t *m_loop;
    zactor_t *m_loop_actor;
    static void actor_thread_func(zsock_t *pipe, void *args);

    //Start section event loop
    void start_client_event_loop();
    
    //Zeromq pipes
    zsock_t *m_stage_requests;		//Reqests sent to the stage server
    zsock_t *m_stage_pipe;          //Stage pipe in/out
    zsock_t *m_graph_out;           //Pub for sending graph updates
    zsock_t *m_graph_in;            //Sub for receiving graph updates
    
    //Socket handlers
    static int s_handle_graph_in(zloop_t *loop, zsock_t *sock, void *arg);
    static int s_handle_reply(zloop_t *loop, zsock_t *sock, void *arg);
    
    //Heartbeat timer
    static int s_heartbeat_timer(zloop_t *loop, int timer_id, void *arg);
    
};

