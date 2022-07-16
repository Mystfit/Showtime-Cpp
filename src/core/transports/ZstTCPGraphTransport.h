#pragma once

#include <showtime/ZstExports.h>
#include "ZstGraphTransport.h"

namespace showtime {

class ZstTCPGraphTransport : 
	public ZstGraphTransport
{
public:
	ZST_EXPORT ZstTCPGraphTransport(boost::asio::io_context& context);
	ZST_EXPORT ~ZstTCPGraphTransport();
	ZST_EXPORT virtual void connect(const std::string & address) override;
	ZST_EXPORT virtual void disconnect() override;
	ZST_EXPORT virtual std::string getPublicIPAddress(struct STUNServer server);

protected:
	ZST_EXPORT virtual void init_graph_sockets() override;


private:
	std::set<std::string> m_connected_addresses;

	std::shared_ptr<boost::asio::ip::tcp::socket> m_tcp_sock;
	uint16_t m_port;
};

}
