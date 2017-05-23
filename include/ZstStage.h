#pragma once
#include "czmq.h"
#include <string>
#include <vector>
#include <iostream>
#include <tuple>
#include <map>
#include <regex>
#include <boost\uuid\uuid.hpp>
#include <boost\uuid\uuid_io.hpp>
#include "ZstActor.h"
#include "ZstExports.h"
#include "ZstMessages.h"
#include "ZstPlug.h"
#include "Showtime.h"

struct ZstPerformerRef{
    std::string name;
    std::string endpoint;
	std::string client_uuid;
    std::vector<PlugAddress> plugs;
};

class ZstStage : public ZstActor{
public:
    //int dealer_port = 6000;
    //int router_port = 6001;
    
    ZST_EXPORT ~ZstStage();
    ZST_EXPORT static ZstStage* create_stage();
    ZST_EXPORT ZstPerformerRef & get_performer_ref(std::string performer_name);
    
private:
    ZstStage();

    //Stage pipes
    zsock_t *m_performer_router;
    zsock_t *m_performer_requests;
    zsock_t *m_graph_update_pub;

    //Incoming router socket handler
    static int s_handle_router(zloop_t *loop, zsock_t *sock, void *arg);
    static int s_handle_performer_requests(zloop_t *loop, zsock_t *sock, void *arg);

    //Message handlers
    void register_performer_handler(zsock_t * socket, zmsg_t * msg);
    void register_plug_handler(zsock_t * socket, zmsg_t * msg);
    void section_heartbeat_handler(zsock_t * socket, zmsg_t * msg);
    void list_plugs_handler(zsock_t * socket, zmsg_t * msg);
    void connect_plugs_handler(zsock_t * socket, zmsg_t * msg);
	void destroy_plug_handler(zsock_t * socket, zmsg_t * msg);
    
    //Graph storage
    std::map<std::string, ZstPerformerRef> m_performer_refs;

	//Plug connections
	void connect_plugs(const ZstPerformerRef & input_performer, const ZstPerformerRef & output_performer, PlugAddress output_plug);
};

