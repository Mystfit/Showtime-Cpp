#pragma once

#include "ZstExports.h"
#include "ZstGraphTransport.h"

namespace showtime {

class ZstTCPGraphTransport : 
	public ZstGraphTransport
{
public:
	ZST_EXPORT ZstTCPGraphTransport();
	ZST_EXPORT ~ZstTCPGraphTransport();
	ZST_EXPORT virtual void connect(const std::string & address) override;
	ZST_EXPORT virtual void disconnect() override;

protected:
	ZST_EXPORT virtual void init_graph_sockets() override;


private:
	std::set<std::string> m_connected_addresses;
};

}
