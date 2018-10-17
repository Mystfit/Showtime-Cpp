#pragma once

#include <ZstExports.h>
#include "ZstGraphTransport.h"

class ZstUDPGraphTransport : 
	public ZstGraphTransport
{
public:
	ZST_EXPORT ZstUDPGraphTransport();
	ZST_EXPORT ~ZstUDPGraphTransport();
	ZST_EXPORT virtual void connect_to_client(const char * endpoint) override;
	ZST_EXPORT virtual void disconnect_from_client() override;

protected:
	ZST_EXPORT virtual void init_graph_sockets() override;
};
