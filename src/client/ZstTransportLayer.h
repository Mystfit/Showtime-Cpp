/*
	ZstTransportLayer

	Base class for transports to communicate with a performance
*/

#pragma once

#include <functional>

#include <ZstCore.h>
#include "ZstClientModule.h"
#include "../core/ZstMessage.h"

//Forwards
class ZstMessageDispatcher;

class ZstTransportLayer : public ZstClientModule
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
	
	virtual ZstMessage * get_msg() = 0;

	virtual void send_to_stage(ZstMessage * msg) = 0;
	virtual void send_to_performance(ZstMessage * msg) = 0;
	virtual ZstMessage * receive_from_stage() = 0;
	virtual void receive_stage_update() = 0;
	virtual void receive_from_performance() = 0;
	
protected:
	std::string m_stage_addr;

	ZstMessageDispatcher * msg_dispatch();
	ZstMessageDispatcher * m_msg_dispatch;
};