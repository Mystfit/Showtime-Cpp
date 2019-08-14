#include "ZstWebsocketServerTransport.h"
#include <boost/asio/ip/tcp.hpp>

ZstWebsocketServerTransport::ZstWebsocketServerTransport(ZstIOLoop& io) :
	m_ioc(io.IO_context()),
	m_acceptor(io.IO_context())
{
}

ZstWebsocketServerTransport::~ZstWebsocketServerTransport()
{
}

void ZstWebsocketServerTransport::init()
{
	beast::error_code ec;
	
	//Start the IO context
	//m_io_thread = boost::thread(boost::ref(m_io));
}

void ZstWebsocketServerTransport::destroy()
{
}

void ZstWebsocketServerTransport::bind(const std::string& address)
{
	beast::error_code ec;

	auto endpoint = tcp::endpoint{ net::ip::make_address(address), 80 };

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

	//Start accepting sockets
	do_accept();
}

void ZstWebsocketServerTransport::send_message_impl(ZstMessage* msg)
{
}

void ZstWebsocketServerTransport::receive_msg(ZstMessage* msg)
{
}

void ZstWebsocketServerTransport::fail(beast::error_code ec, char const* what)
{
	ZstLog::server(LogLevel::error, "Websocket transport error: {} {}", what, ec.message());
}

void ZstWebsocketServerTransport::do_accept()
{
	// The new connection gets its own strand
	m_acceptor.async_accept(
		net::make_strand(m_ioc),
		beast::bind_front_handler(&ZstWebsocketServerTransport::on_accept, shared_from_this())
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
		std::make_shared<ZstWebsocketSession>(std::move(socket))->run();
	}

	// Accept another connection
	do_accept();
}


//--------------------------------


ZstWebsocketSession::ZstWebsocketSession(tcp::socket&& socket) : m_ws(std::move(socket))
{
}

void ZstWebsocketSession::run()
{
	// Set suggested timeout settings for the websocket
	m_ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

	// Set a decorator to change the Server of the handshake
	m_ws.set_option(websocket::stream_base::decorator([](websocket::response_type& res){
		res.set(http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async");
	}));

	// Accept the websocket handshake
	m_ws.async_accept(beast::bind_front_handler(&ZstWebsocketSession::on_accept, shared_from_this()));
}

void ZstWebsocketSession::on_accept(beast::error_code ec)
{
	if (ec)
		return ZstWebsocketServerTransport::fail(ec, "accept");

	// Read a message
	do_read();
}

void ZstWebsocketSession::do_read()
{
	// Read a message into our buffer
	m_ws.async_read(m_buffer, beast::bind_front_handler(&ZstWebsocketSession::on_read, shared_from_this()));
}

void ZstWebsocketSession::on_read(beast::error_code ec, std::size_t bytes_transferred)
{
	boost::ignore_unused(bytes_transferred);

	// This indicates that the session was closed
	if (ec == websocket::error::closed)
		return;

	if (ec)
		ZstWebsocketServerTransport::fail(ec, "read");

	ZstLog::net(LogLevel::debug, "Websocket received message '{}'", beast::buffers_to_string(m_buffer.data()));

	// Echo the message
	m_ws.text(m_ws.got_text());
	m_ws.async_write(m_buffer.data(), beast::bind_front_handler(&ZstWebsocketSession::on_write, shared_from_this()));
}

void ZstWebsocketSession::on_write(beast::error_code ec, std::size_t bytes_transferred)
{
	boost::ignore_unused(bytes_transferred);

	if (ec)
		return ZstWebsocketServerTransport::fail(ec, "write");

	// Clear the buffer
	m_buffer.consume(m_buffer.size());

	// Do another read
	do_read();
}
