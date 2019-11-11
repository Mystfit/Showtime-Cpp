#pragma once

#include "ZstWebsocketServerTransport.h"
#include <string>
#include <boost/uuid/uuid.hpp>

using namespace boost::uuids;

namespace showtime {

class ZstWebsocketSession : public std::enable_shared_from_this<ZstWebsocketSession> {
public:
	// Take ownership of the socket and the owning transport
	explicit ZstWebsocketSession(tcp::socket&& socket, std::shared_ptr<ZstWebsocketServerTransport> transport);
	~ZstWebsocketSession();
	void run();

	void on_accept(beast::error_code ec);
	void do_read();
	void do_write(const uint8_t* msg_buffer, size_t msg_buffer_size);
	void on_send(std::shared_ptr<std::pair< std::unique_ptr<uint8_t>, size_t > const> const& msg_data);
	void on_read(beast::error_code ec, std::size_t bytes_transferred);
	void on_write(beast::error_code ec, std::size_t bytes_transferred);

	const uuid& endpoint_UUID();
private:
	websocket::stream<beast::tcp_stream> m_ws;
	std::string m_socket_id;
	beast::flat_buffer m_recv_buffer;
	std::vector<std::shared_ptr< std::pair< std::unique_ptr<uint8_t>, size_t > const> > m_out_messages;
	std::shared_ptr<ZstWebsocketServerTransport> m_transport;
	uuid m_endpoint_UUID;
};

}