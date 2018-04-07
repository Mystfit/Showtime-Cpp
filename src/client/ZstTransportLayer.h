/*
	ZstTransportLayer

	Base class for transports to communicate with a performance
*/

#pragma once

#include <ZstCore.h>
#include "../core/ZstMessage.h"
#include "ZstClientModule.h"

class ZstTransportLayer : public ZstClientModule {
public:
	ZstTransportLayer(ZstClient * client);
	~ZstTransportLayer();
	virtual void destroy() override {};
	virtual void init() override {}
	virtual void connect_to_stage(std::string stage_address) = 0;
	virtual void disconnect_from_stage() = 0;
	
	// ---------------
	// Debugging
	// ---------------
	
	int graph_recv_tripmeter();
	void reset_graph_recv_tripmeter();
	int graph_send_tripmeter();
	void reset_graph_send_tripmeter();

	// ---------------
	// Message IO
	// ---------------
	virtual ZstMessage * get_msg() = 0;
	virtual void send_to_stage(ZstMessage * msg) = 0;
	virtual ZstMessage * receive_from_stage() = 0;
	virtual ZstMessage * receive_stage_update() = 0;
	virtual void send_to_performance(ZstPlug * plug) = 0;

protected:
	std::string m_stage_addr;

private:
	ZstTransportLayer();
	void inc_graph_recv();
	void inc_graph_send();

	int m_num_graph_recv_messages;
	int m_num_graph_send_messages;
};