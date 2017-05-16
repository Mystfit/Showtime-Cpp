#pragma once
#include "ZstExports.h"
#include "ZstMessages.hpp"
#include "ZstPlug.h"
#include "ZstPerformance.h"
#include "czmq.h"
#include <string>
#include <vector>
#include <iostream>
#include <tuple>
#include <map>
#include <regex>

struct ZstPerformerRef{
    std::string name;
    zsock_t * pipe;
    std::vector<ZstPlugAddress> plugs;
};

class ZstStage {
public:
    //int dealer_port = 6000;
    //int router_port = 6001;
    
    ZST_EXPORT ~ZstStage();
    ZST_EXPORT static ZstStage* create_stage();
    ZST_EXPORT ZstPerformerRef get_performer_ref(std::string performer_name);
    
private:
    ZstStage();
    
    //Client actors
    zloop_t *m_loop;
    zactor_t *m_loop_actor;
    static void actor_thread_func(zsock_t *pipe, void *args);
    
    //Stage pipes
    zsock_t *m_section_router;
    zsock_t *m_graph_update_pub;
    
    //Let's get it started in HAH
    void start_server_event_loop();

    //Incoming router socket handler
    static int s_handle_router(zloop_t *loop, zsock_t *sock, void *arg);
    static int s_handle_section_pipe(zloop_t *loop, zsock_t *sock, void *arg);

    //Message handlers
    void register_section_handler(zsock_t * socket, zframe_t * identity, zmsg_t * msg);
    void register_plug_handler(zsock_t * socket, zframe_t * identity, zmsg_t * msg);
    void section_heartbeat_handler(zsock_t * socket, zframe_t * identity, zmsg_t * msg);
    void list_plugs_handler(zsock_t * socket, zframe_t * identity, zmsg_t * msg);
    void connect_plugs_handler(zsock_t * socket, zframe_t * identity, zmsg_t * msg);
    
    //Acks
    void send_section_heartbeat_ack(zsock_t * socket, zframe_t * identity);
    
    //Graph storage
    std::map<std::string, ZstPerformerRef> m_performer_refs;
};

