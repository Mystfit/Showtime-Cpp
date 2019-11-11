#include "ZstWebsocketServerTransport.h"
#include "ZstWebsocketSession.h"

#include <boost/asio/ip/tcp.hpp>
#include <nlohmann/json.hpp>

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
}

void ZstWebsocketServerTransport::destroy()
{
	ZstTransportLayerBase::destroy();
}

void ZstWebsocketServerTransport::bind(const std::string& address)
{
	beast::error_code ec;

	auto endpoint = tcp::endpoint{ net::ip::make_address(address), 40005 };

	// Open the acceptor
	m_acceptor.open(endpoint.protocol(), ec);
	if (ec) {
		ZstWebsocketServerTransport::fail(ec, "open");
		return;
	}

	// Bind to the server address
	m_acceptor.bind(endpoint, ec);
	if (ec) {
		ZstWebsocketServerTransport::fail(ec, "bind");
		return;
	}

	// Allow address reuse
	m_acceptor.set_option(net::socket_base::reuse_address(true), ec);
	if (ec) {
		ZstWebsocketServerTransport::fail(ec, "set_option");
		return;
	}

	// Start listening for connections
	m_acceptor.listen(net::socket_base::max_listen_connections, ec);
	if (ec) {
		ZstWebsocketServerTransport::fail(ec, "listen");
		return;
	}

	ZstLog::server(LogLevel::notification, "Websocket transport listening on port {}", endpoint.port());

	//Start accepting sockets
	do_accept();
}

void ZstWebsocketServerTransport::fail(beast::error_code ec, char const* what)
{
	ZstLog::server(LogLevel::error, "Websocket transport error: {} {}", what, ec.message());
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
		beast::bind_front_handler(&ZstWebsocketServerTransport::on_accept, ZstWebsocketServerTransport::downcasted_shared_from_this<ZstWebsocketServerTransport>())
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
		ZstLog::net(LogLevel::debug, "Websocket received new connection from {}", socket.remote_endpoint().address().to_string());

		// Create the session and run it
		auto session = std::make_shared<ZstWebsocketSession>(std::move(socket), ZstWebsocketServerTransport::downcasted_shared_from_this<ZstWebsocketServerTransport>());
		session->run();
		m_sessions.insert(std::pair<uuid, ZstWebsocketSessionPtr>(session->endpoint_UUID(), session));
	}

	// Accept another connection
	do_accept();
}

}