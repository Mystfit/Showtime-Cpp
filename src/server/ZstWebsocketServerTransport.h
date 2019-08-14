#pragma once

#include "../core/ZstIOLoop.h"
#include "../core/ZstStageMessage.h"
#include "../core/transports/ZstTransportLayer.h"

#include <boost/thread.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


class ZstWebsocketServerTransport :
	public ZstTransportLayer<ZstStageMessage>,
	public std::enable_shared_from_this<ZstWebsocketServerTransport>
{
public:
	ZstWebsocketServerTransport(ZstIOLoop & io);
	~ZstWebsocketServerTransport();
	void init() override;
	void destroy() override;
	virtual void bind(const std::string& address) override;

	void send_message_impl(ZstMessage* msg) override;
	void receive_msg(ZstMessage* msg) override;

	static void fail(beast::error_code ec, char const* what);

private:
	void do_accept();
	void on_accept(beast::error_code ec, tcp::socket socket);

	boost::thread m_io_thread;
	tcp::acceptor m_acceptor;
	boost::asio::io_context& m_ioc;
};


class ZstWebsocketSession : public std::enable_shared_from_this<ZstWebsocketSession> {
public:
	// Take ownership of the socket
	explicit ZstWebsocketSession(tcp::socket&& socket);

	void run();

	void on_accept(beast::error_code ec);
	void do_read();
	void on_read(beast::error_code ec, std::size_t bytes_transferred);
	void on_write(beast::error_code ec, std::size_t bytes_transferred);
private:
	websocket::stream<beast::tcp_stream> m_ws;
	beast::flat_buffer m_buffer;
};