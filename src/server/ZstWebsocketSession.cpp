#include "ZstWebsocketSession.h"
#include <boost/uuid/random_generator.hpp>

namespace showtime {

ZstWebsocketSession::ZstWebsocketSession(tcp::socket&& socket, std::shared_ptr<ZstWebsocketServerTransport> transport) :
	m_ws(std::move(socket)),
	m_transport(transport)
{
	m_origin_endpoint_UUID = random_generator()();
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

	m_ws.binary(true);

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
	//TODO: Create a pool of buffer objects
	m_ws.async_read(m_recv_buffer, beast::bind_front_handler(&ZstWebsocketSession::on_read, shared_from_this()));
}

void ZstWebsocketSession::on_read(beast::error_code ec, std::size_t bytes_transferred){
	boost::ignore_unused(bytes_transferred);

	// This indicates that the session was closed
	if (ec == websocket::error::closed) {
		Log::net(Log::Level::debug, "Websocket closed");
		return;
	}

	if (ec) {
		ZstWebsocketServerTransport::fail(ec, "read");
		return;
	}

	Log::net(Log::Level::debug, "Websocket received message '{}'", beast::buffers_to_string(m_recv_buffer.data()));

	if (!m_ws.got_binary()) {
		ZstWebsocketServerTransport::fail(ec, "Websocket received non-binary message");

		do_read();
		return;
	}

	//Parse message
	auto msg = m_transport->get_msg();
	auto uuid = boost::uuids::nil_uuid();
	auto owner = std::dynamic_pointer_cast<ZstStageTransport>(m_transport->ZstTransportLayer::shared_from_this());
	msg->init(GetStageMessage(
		m_recv_buffer.data().data()),
		m_origin_endpoint_UUID,
		uuid,
		owner
	);

	// Send message to other modules
	m_transport->dispatch_receive_event(msg, [this](ZstEventStatus e) {
		//Clear the buffer
		m_recv_buffer.consume(m_recv_buffer.size());

		//Read another
		do_read();
	});
}

void ZstWebsocketSession::do_write(const uint8_t* msg_buffer, size_t msg_buffer_size){
	// Copy the message contents since we don't want to lose hem if the flatbuffer builder disappears
	// TODO: Replace with detatchedbuffer?
	auto data = std::make_unique<uint8_t[]>(msg_buffer_size);
	std::copy(msg_buffer, msg_buffer + msg_buffer_size, data.get());
	auto pair = std::make_shared<std::pair<std::unique_ptr<uint8_t[]>, size_t > >(std::move(data), msg_buffer_size);
	net::post(m_ws.get_executor(), beast::bind_front_handler(&ZstWebsocketSession::on_send, shared_from_this(), pair));
}

void ZstWebsocketSession::on_send(std::shared_ptr< std::pair<std::unique_ptr<uint8_t[]>, size_t > const > const& msg_data) {
	// Always add to queue
	m_out_messages.push_back(msg_data);

	// Are we already writing?
	if (m_out_messages.size() > 1)
		return;

	// We are not currently writing, so send this immediately
	m_ws.async_write(net::buffer(m_out_messages.front()->first.get(), m_out_messages.front()->second), beast::bind_front_handler(&ZstWebsocketSession::on_write, shared_from_this()));
}

void ZstWebsocketSession::on_write(beast::error_code ec, std::size_t bytes_transferred){
	boost::ignore_unused(bytes_transferred);

	if (ec)
		return ZstWebsocketServerTransport::fail(ec, "write");

	// Remove the message from the queue
	m_out_messages.erase(m_out_messages.begin());

	// Send the next message if any
	if (!m_out_messages.empty()) {
		m_ws.async_write(net::buffer(m_out_messages.front()->first.get(), m_out_messages.front()->second), beast::bind_front_handler(&ZstWebsocketSession::on_write, shared_from_this()));
	}
}

const uuid& ZstWebsocketSession::origin_endpoint_UUID()
{
	return m_origin_endpoint_UUID;
}

}