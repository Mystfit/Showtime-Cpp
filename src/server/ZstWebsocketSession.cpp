#pragma once

#include "ZstWebsocketSession.h"
#include <czmq.h>

ZstWebsocketSession::ZstWebsocketSession(tcp::socket&& socket, std::shared_ptr<ZstWebsocketServerTransport> transport) :
	m_ws(std::move(socket)),
	m_transport(transport)
{
	zuuid_t* uuid = zuuid_new();
	m_socket_id = std::string(zuuid_str_canonical(uuid));
	zuuid_destroy(&uuid);
}

ZstWebsocketSession::~ZstWebsocketSession()
{
	beast::websocket::close_reason reason;
	m_ws.close(reason);
}

void ZstWebsocketSession::run()
{
	// Set suggested timeout settings for the websocket
	m_ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

	// Set a decorator to change the Server of the handshake
	m_ws.set_option(websocket::stream_base::decorator([](websocket::response_type& res) {
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
	m_ws.async_read(m_recv_buffer, beast::bind_front_handler(&ZstWebsocketSession::on_read, shared_from_this()));
}

void ZstWebsocketSession::on_read(beast::error_code ec, std::size_t bytes_transferred)
{
	boost::ignore_unused(bytes_transferred);

	// This indicates that the session was closed
	if (ec == websocket::error::closed) {
		ZstLog::net(LogLevel::debug, "Websocket closed");
		return;
	}

	if (ec) {
		ZstWebsocketServerTransport::fail(ec, "read");
		return;
	}

	ZstLog::net(LogLevel::debug, "Websocket received message '{}'", beast::buffers_to_string(m_recv_buffer.data()));

	//Parse message
	ZstStageMessage* msg = m_transport->get_msg();
	std::string msg_str = beast::buffers_to_string(m_recv_buffer.data());
	json msg_json;

	try {
		msg_json = json::parse(msg_str);
	}
	catch (nlohmann::detail::parse_error) {
		ZstLog::net(LogLevel::debug, "Could not parse json message '{}'", msg_str);
	}
	
	//Unpack message
	if (!msg_json.empty()) {
		msg->unpack(msg_json);
		msg->set_arg<std::string, std::string>(get_msg_arg_name(ZstMsgArg::SENDER), m_socket_id);
		m_transport->receive_msg(msg);
	}

	//Clear the buffer
	m_recv_buffer.consume(m_recv_buffer.size());

	//Read another
	do_read();
}

void ZstWebsocketSession::do_write(const std::string& data) 
{
	std::shared_ptr<std::string const> msg_data(std::make_shared<std::string const>(data));
	net::post(m_ws.get_executor(), beast::bind_front_handler(&ZstWebsocketSession::on_send, shared_from_this(), msg_data));
}

void ZstWebsocketSession::on_send(std::shared_ptr<std::string const> const& msg_data) {
	// Always add to queue
	m_out_messages.push_back(msg_data);

	// Are we already writing?
	if (m_out_messages.size() > 1)
		return;

	// We are not currently writing, so send this immediately
	m_ws.async_write(net::buffer(*m_out_messages.front()),beast::bind_front_handler(&ZstWebsocketSession::on_write, shared_from_this()));
}

void ZstWebsocketSession::on_write(beast::error_code ec, std::size_t bytes_transferred)
{
	boost::ignore_unused(bytes_transferred);

	if (ec)
		return ZstWebsocketServerTransport::fail(ec, "write");

	// Remove the string from the queue
	m_out_messages.erase(m_out_messages.begin());

	// Send the next message if any
	if (!m_out_messages.empty()) {
		m_ws.async_write(net::buffer(*m_out_messages.front()), beast::bind_front_handler(&ZstWebsocketSession::on_write, shared_from_this()));
	}
}

const std::string& ZstWebsocketSession::get_id()
{
	return m_socket_id;
}
