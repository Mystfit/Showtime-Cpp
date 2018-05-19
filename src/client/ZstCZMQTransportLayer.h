#pragma once

#include <functional>
#include <string>
#include <czmq.h>

#include "../core/ZstActor.h"
#include "../core/ZstMessagePool.hpp"
#include "../core/ZstStageMessage.h"
#include "../core/ZstPerformanceMessage.h"
#include "ZstTransportLayer.h"

//Forwards
class ZstMessageDispatcher;

class ZstCZMQTransportLayer : 
	public ZstTransportLayer, 
	public ZstActor
{
public:
	ZstCZMQTransportLayer();
	~ZstCZMQTransportLayer();
	virtual void destroy() override;
	virtual void init() override;
	void connect_to_stage(std::string stage_address) override;
	void connect_to_client(const char * endpoint_ip, const char * subscription_plug) override;
	void disconnect_from_stage() override;

	int add_timer(int delay, std::function<void()> timer_func);
	void remove_timer(int timer_id);

	const char * get_graph_address();

	ZstStageMessage * get_stage_msg() override;
	ZstPerformanceMessage * get_performance_msg() override;

private:	
	// ---------------
	// Socket handlers
	// ---------------

	static int s_handle_graph_in(zloop_t *loop, zsock_t *sock, void *arg);
	static int s_handle_stage_update_in(zloop_t *loop, zsock_t *sock, void *arg);
	static int s_handle_stage_router(zloop_t *loop, zsock_t *sock, void *arg);
	static int s_handle_timer(zloop_t * loop, int timer_id, void * arg);


	// ---------------
	// Timers
	// ---------------
	
	std::unordered_map<int, std::function<void()> > m_timers;

	// ---------------
	// Message IO
	// ---------------
	
	void send_to_stage(ZstStageMessage * msg) override;
	void send_to_performance(ZstPerformanceMessage * msg) override;

	zmsg_t * sock_recv(zsock_t* socket, bool pop_first);
	ZstStageMessage * receive_addressed_msg() override;
	void receive_stage_update() override;
	void receive_from_performance() override;
	
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

	ZstMessagePool<ZstStageMessage> m_stage_msg_pool;
	ZstMessagePool<ZstPerformanceMessage> m_performance_msg_pool;

};
