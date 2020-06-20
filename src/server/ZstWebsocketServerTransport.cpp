#include "ZstWebsocketServerTransport.h"
#include "ZstWebsocketSession.h"
#include <showtime/ZstConstants.h>

#include <boost/asio/ip/tcp.hpp>

namespace showtime {

ZstWebsocketServerTransport::ZstWebsocketServerTransport(ZstIOLoop& io) :
	m_ioc(io.IO_context()),
	m_acceptor(io.IO_context())
{
}

ZstWebsocketServerTransport::~ZstWebsocketServerTransport()
{
	m_sessions.clear();
}

void ZstWebsocketServerTransport::init()
{
	set_connected(true);
}

void ZstWebsocketServerTransport::destroy()
{
	set_connected(false);
	ZstTransportLayer::destroy();
}

int ZstWebsocketServerTransport::bind(const std::string& address)
{
	beast::error_code ec;
	int port = STAGE_WEBSOCKET_PORT;

	auto endpoint = tcp::endpoint{ net::ip::make_address(address), static_cast<unsigned short>(port) };

	// Open the acceptor
	m_acceptor.open(endpoint.protocol(), ec);
	if (ec) {
		ZstWebsocketServerTransport::fail(ec, "open");
		return -1;
	}

	// Bind to the server address
	m_acceptor.bind(endpoint, ec);
	if (ec) {
		ZstWebsocketServerTransport::fail(ec, "bind");
		return -1;
	}

	// Allow address reuse
	m_acceptor.set_option(net::socket_base::reuse_address(true), ec);
	if (ec) {
		ZstWebsocketServerTransport::fail(ec, "set_option");
		return -1;
	}

	// Start listening for connections
	m_acceptor.listen(net::socket_base::max_listen_connections, ec);
	if (ec) {
		ZstWebsocketServerTransport::fail(ec, "listen");
		return -1;
	}

	Log::server(Log::Level::debug, "Websocket transport listening on port {}", endpoint.port());

	//Start accepting sockets
	do_accept();

	return port;
}

void ZstWebsocketServerTransport::fail(beast::error_code ec, char const* what)
{
	Log::server(Log::Level::error, "Websocket transport error: {} {}", what, ec.message());
}

void ZstWebsocketServerTransport::send_message_impl(const uint8_t* msg_buffer, size_t msg_buffer_size, const ZstTransportArgs& args) const
{
	auto session = m_sessions.find(args.target_endpoint_UUID);
	if (session != m_sessions.end()) {
		session->second->do_write(msg_buffer, msg_buffer_size);
	}
}

void ZstWebsocketServerTransport::do_accept()
{
	// The new connection gets its own strand
	m_acceptor.async_accept(
		net::make_strand(m_ioc),
		beast::bind_front_handler(&ZstWebsocketServerTransport::on_accept, ZstTransportLayer::downcasted_shared_from_this<ZstWebsocketServerTransport>())
	);
}

void ZstWebsocketServerTransport::on_accept(beast::error_code ec, tcp::socket socket)
{
	if (ec)
	{
		ZstWebsocketServerTransport::fail(ec, "accept");
	}
	else
	{
		Log::net(Log::Level::debug, "Websocket received new connection from {}", socket.remote_endpoint().address().to_string());

		// Create the session and run it
		auto session = std::make_shared<ZstWebsocketSession>(std::move(socket), ZstTransportLayer::downcasted_shared_from_this<ZstWebsocketServerTransport>());
		session->run();
		m_sessions.insert(std::pair<uuid, ZstWebsocketSessionPtr>(session->origin_endpoint_UUID(), session));
	}

	// Accept another connection
	do_accept();
}

}