#include <boost/asio/connect.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include "ZstHTTPTransport.h"

ZstHTTPTransport::ZstHTTPTransport(boost::asio::io_context& io_context) :
	m_resolver(io_context),
	m_socket(io_context)
{
}

ZstHTTPTransport::~ZstHTTPTransport()
{
}

void ZstHTTPTransport::init()
{
}

void ZstHTTPTransport::destroy()
{
}

void ZstHTTPTransport::connect_to_stage(std::string stage_address)
{
	// Look up the domain name
	m_resolver.async_resolve(host, port, std::bind(
			&ZstHTTPTransport::on_resolve,
			shared_from_this(),
			std::placeholders::_1,
			std::placeholders::_2)
	);
}

void ZstHTTPTransport::disconnect_from_stage()
{
}

void ZstHTTPTransport::send_message_impl(ZstMessage * msg)
{
}

void ZstHTTPTransport::on_receive_msg(ZstMessage * msg)
{
}

void ZstHTTPTransport::on_resolve(boost::system::error_code ec, tcp::resolver::results_type results)
{
	if (ec) return fail(ec, "resolve");

	// Make the connection on the IP address we get from a lookup
	boost::asio::async_connect(
		m_socket,
		results.begin(),
		results.end(),
		std::bind(
			&ZstHTTPTransport::on_connect,
			shared_from_this(),
			std::placeholders::_1));
}

void ZstHTTPTransport::on_connect(boost::system::error_code ec)
{
	if (ec) return fail(ec, "connect");

	http::request<http::empty_body> req;
	// Set up an HTTP GET request message
	req.version(1);
	req.method(http::verb::get);
	req.target(target);
	req.set(http::field::host, host);
	req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

	// Send the HTTP request to the remote host
	http::async_write(m_socket, req,
		std::bind(
			&ZstHTTPTransport::on_write,
			shared_from_this(),
			std::placeholders::_1,
			std::placeholders::_2));
}

void ZstHTTPTransport::on_write(boost::system::error_code ec, std::size_t bytes_transferred)
{
	boost::ignore_unused(bytes_transferred);

	if (ec)
		return fail(ec, "write");

	// Receive the HTTP response
	http::async_read(m_socket, m_buffer, res_,
		std::bind(
			&ZstHTTPTransport::on_read,
			shared_from_this(),
			std::placeholders::_1,
			std::placeholders::_2));
}

void ZstHTTPTransport::on_read(boost::system::error_code ec, std::size_t bytes_transferred)
{
}

void ZstHTTPTransport::fail(boost::system::error_code ec, char const * what)
{
	std::cerr << what << ": " << ec.message() << "\n";
}
