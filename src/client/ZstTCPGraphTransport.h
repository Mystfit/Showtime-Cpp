#pragma once

#include "../core/liasons/ZstPlugLiason.hpp"
#include "../core/ZstTransportLayer.h"
#include "../core/ZstPerformanceMessage.h"
#include "ZstGraphTransport.h"

class ZstTCPGraphTransport : 
	public ZstGraphTransport
{
public:
	ZstTCPGraphTransport();
	~ZstTCPGraphTransport();
	virtual void connect_to_client(const char * endpoint) override;
	virtual void disconnect_from_client() override;

protected:
	virtual void init_graph_sockets() override;
};
