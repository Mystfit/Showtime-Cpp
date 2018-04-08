/*
	ZstTransportLayer

	Base class for transports to communicate with a performance
*/

#pragma once

#include <ZstCore.h>
#include "../core/ZstMessage.h"
#include "ZstClientModule.h"
#include "ZstClientModule.h"

class ZstTransportLayer : public ZstClientModule {
public:
	ZstTransportLayer(ZstClient * client);
	~ZstTransportLayer();
	virtual void destroy() override {};
	virtual void init() override {}
	virtual void connect_to_stage(std::string stage_address) = 0;
	virtual void disconnect_from_stage() = 0;

	// -----------------
	// Peer connectivity
	// -----------------
	virtual void connect_to_client(const char * endpoint_ip, const char * subscription_plug) = 0;


	// ---------------
	// Message IO
	// ---------------
	virtual ZstMessage * get_msg() = 0;
	virtual void send_to_stage(ZstMessage * msg) = 0;
	virtual ZstMessage * receive_from_stage() = 0;
	virtual ZstMessage * receive_stage_update() = 0;
	virtual void send_to_performance(ZstMessage * msg) = 0;
	virtual void receive_from_performance() = 0;

protected:
	std::string m_stage_addr;

private:
	ZstTransportLayer();
};