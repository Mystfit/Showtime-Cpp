#pragma once

#include <string>
#include <czmq.h>
#include "ZstTransportLayer.h"

class ZstCZMQTransportLayer : public ZstTransportLayer {

public:
	ZstCZMQTransportLayer();
	~ZstCZMQTransportLayer();

private:
	// ---------------
	// Socket handlers
	// ---------------

	static int s_handle_graph_in(zloop_t *loop, zsock_t *sock, void *arg);
	static int s_handle_stage_update_in(zloop_t *loop, zsock_t *sock, void *arg);
	static int s_handle_stage_router(zloop_t *loop, zsock_t *sock, void *arg);


	// ---------------
	// Message IO
	// ---------------
	
	void send_to_stage(ZstMessage * msg);
	ZstMessage * receive_from_stage();
	ZstMessage * receive_stage_update();
	void send_to_performance(ZstPlug * plug);


	// ---------------
	// ZMQ Sockets
	// ---------------

	zsock_t * m_stage_router;
	zsock_t * m_stage_updates;
	zsock_t * m_graph_out_tcp;
	zsock_t * m_graph_out_udp;

	// ---------------
	// Addresses
	// ---------------
	
	std::string first_available_ext_ip();

	std::string m_stage_router_addr;
	std::string m_stage_updates_addr;
	std::string m_graph_out_addr;
	std::string m_graph_out_ip;
	std::string m_network_interface;
};
