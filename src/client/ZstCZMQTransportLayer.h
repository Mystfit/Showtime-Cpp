#pragma once

#include <string>
#include <czmq.h>
#include "ZstTransportLayer.h"
#include "ZstCZMQMessage.h"

class ZstCZMQTransportLayer : public ZstTransportLayer {

public:
	ZstCZMQTransportLayer(ZstClient * client);
	~ZstCZMQTransportLayer();
	virtual void destroy() override;
	virtual void init() override;
	void connect_to_stage(std::string stage_address) override;
	void connect_to_client(const char * endpoint_ip, const char * subscription_plug);
	void disconnect_from_stage();
	ZstMessage * get_msg() override;

private:
	ZstCZMQTransportLayer();
	
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
	void send_to_performance(ZstMessage * msg);
	void receive_from_performance();

	// ---------------
	// ZMQ Sockets
	// ---------------

	zsock_t * m_stage_router;
	zsock_t * m_stage_updates;
	zsock_t * m_graph_in;
	zsock_t * m_graph_out;

	// ---------------
	// Addresses
	// ---------------
	
	std::string first_available_ext_ip();

	std::string m_stage_router_addr;
	std::string m_stage_updates_addr;
	std::string m_graph_out_addr;
	std::string m_graph_out_ip;
	std::string m_network_interface;

	zuuid_t * m_startup_uuid;


	/** Summary:	The message pool. */
	ZstMessagePool<ZstCZMQMessage> m_pool;
};
