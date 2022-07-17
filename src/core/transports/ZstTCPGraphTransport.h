#pragma once

#include <showtime/ZstExports.h>
#include "ZstGraphTransport.h"

namespace showtime {

	// Forwards
	class ZstTCPSession;

class ZstTCPGraphTransport : 
	public ZstGraphTransport
{
public:
	ZST_EXPORT ZstTCPGraphTransport(boost::asio::io_context& context);
	ZST_EXPORT ~ZstTCPGraphTransport();
	ZST_EXPORT virtual int bind(const std::string& address) override;
	ZST_EXPORT virtual void listen() override;
	ZST_EXPORT virtual void connect(const std::string & address) override;
	ZST_EXPORT virtual void disconnect() override;
	ZST_EXPORT virtual std::string getPublicIPAddress(struct STUNServer server);

protected:
	ZST_EXPORT virtual void init_graph_sockets() override;
	ZST_EXPORT void handle_accept(const boost::system::error_code& error, boost::asio::ip::tcp::socket socket);

private:
	std::shared_ptr<boost::asio::ip::tcp::acceptor> m_acceptor;
	std::shared_ptr<boost::asio::ip::tcp::socket> m_tcp_sock;
	std::unordered_map< uuid, std::shared_ptr<ZstTCPSession>, boost::hash<boost::uuids::uuid> > m_connections;

	uint16_t m_port;
};

}
