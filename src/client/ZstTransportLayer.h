/*
	ZstTransportLayer

	Base class for transports to communicate with a performance
*/

#pragma once

#include <functional>

#include <ZstCore.h>
#include "ZstClientModule.h"
#include "../core/ZstStageMessage.h"
#include "../core/ZstPerformanceMessage.h"

//Forwards
class ZstMessageDispatcher;

class ZstTransportLayer : 
	public ZstClientModule,
	public ZstPlugLiason
{
public:
	ZstTransportLayer();
	~ZstTransportLayer();
	void set_dispatcher(ZstMessageDispatcher * dispatcher);

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
	virtual ZstMessage * receive_addressed_msg() = 0;
	virtual void receive_stage_update() = 0;
	virtual void receive_from_performance() = 0;
	
protected:
	std::string m_stage_addr;

	ZstMessageDispatcher * msg_dispatch();
	ZstMessageDispatcher * m_msg_dispatch;
};