#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "../core/adaptors/ZstTransportAdaptor.hpp"
#include "../core/transports/ZstTransportLayer.h"
#include "../core/ZstStageMessage.h"

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

class ZstHTTPTransport : 
	public ZstTransportLayer<ZstStageMessage>,
	public std::enable_shared_from_this<ZstHTTPTransport>
{
public:
	ZstHTTPTransport(boost::asio::io_context& io_context);
	~ZstHTTPTransport();
	virtual void init() override;
	virtual void destroy() override;
	void connect_to_stage(std::string stage_address);
	void disconnect_from_stage();

private:
	void send_message_impl(ZstMessage * msg) override;
	void on_receive_msg(ZstMessage * msg) override;

	//HTTP callbacks
	void on_resolve(boost::system::error_code ec, tcp::resolver::results_type results);
	void on_connect(boost::system::error_code ec);
	void on_write(boost::system::error_code ec, std::size_t bytes_transferred);
	void on_read(boost::system::error_code ec, std::size_t bytes_transferred);

	void fail(boost::system::error_code ec, char const* what);

	tcp::resolver m_resolver;
	tcp::socket m_socket;
	boost::beast::flat_buffer m_buffer;
};
