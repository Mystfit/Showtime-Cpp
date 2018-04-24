/*
	ZstTransportLayer

	Base class for transports to communicate with a performance
*/

#pragma once

#include <functional>

#include <ZstCore.h>
#include "../core/ZstMessage.h"

class ZstTransportLayer : 
{
public:
	ZstTransportLayer(ZstMessageDispatcher * dispatcher);
	~ZstTransportLayer();
	virtual void destroy() override {};
	virtual void init() override {}
	virtual void connect_to_stage(std::string stage_address) = 0;
	virtual void disconnect_from_stage() = 0;


	// -----------------
	// Peer connectivity
	// -----------------
	// 
	virtual void connect_to_client(const char * endpoint_ip, const char * subscription_plug) = 0;


	// ---------------
	// Message IO
	// ---------------
	
	virtual ZstStageMessage * get_stage_msg() = 0;
	virtual ZstPerformanceMessage * get_performance_msg() = 0;

	virtual void send_to_stage(ZstStageMessage * msg) = 0;
	virtual void send_to_performance(ZstPerformanceMessage * msg) = 0;
	virtual ZstStageMessage * receive_from_stage() = 0;
	virtual ZstStageMessage * receive_stage_update() = 0;
	virtual ZstPerformanceMessage * receive_from_performance() = 0;

protected:
	std::string m_stage_addr;

	ZstMessageDispatcher * msg_dispatch();
	ZstMessageDispatcher m_msg_dispatch;

private:
	ZstTransportLayer();
};