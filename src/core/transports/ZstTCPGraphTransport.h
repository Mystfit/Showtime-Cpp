#pragma once

#include <showtime/ZstExports.h>
#include <showtime/ZstURI.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
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
	ZST_EXPORT virtual std::string getPublicIPAddress(struct STUNServer server) override;
	ZST_EXPORT bool is_connected_to(const ZstURI& client);

protected:
	ZST_EXPORT virtual void init_graph_sockets() override;
	ZST_EXPORT void handle_accept(const boost::system::error_code& error, boost::asio::ip::tcp::socket& socket);
	ZST_EXPORT virtual void send_message_impl(std::shared_ptr<flatbuffers::FlatBufferBuilder> buffer_builder, const ZstTransportArgs& args) const override;

private:
	boost::asio::io_context& m_ctx;	
	std::vector< std::shared_ptr<ZstTCPSession> > m_pending_graph_connections;
	std::unordered_map<ZstURI, std::shared_ptr<ZstTCPSession>, ZstURIHash> m_graph_connections;
};

}
