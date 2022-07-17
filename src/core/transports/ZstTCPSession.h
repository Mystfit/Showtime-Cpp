#pragma once

#include <memory>
#include <string>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/uuid/uuid.hpp>
#include "ZstTCPGraphTransport.h"

using namespace boost::uuids;

namespace showtime {

	class ZstTCPSession : public std::enable_shared_from_this<ZstTCPSession> {
	public:
		// Take ownership of the socket and the owning transport
		explicit ZstTCPSession(boost::asio::ip::tcp::socket&& socket, std::shared_ptr<ZstTCPGraphTransport> transport);
		~ZstTCPSession();
		void run();

		void on_accept(const boost::system::error_code& error);
		void do_read();
		void do_write(const uint8_t* msg_buffer, size_t msg_buffer_size);
		void on_send(std::shared_ptr<std::pair< std::unique_ptr<uint8_t[]>, size_t > const> const& msg_data);
		void on_read(const boost::system::error_code& error, std::size_t bytes_transferred);
		void on_write(const boost::system::error_code& error, std::size_t bytes_transferred);

		const uuid& origin_endpoint_UUID();
	private:
		boost::asio::ip::tcp m_socket;

		boost::array<char, 1024> m_recv_buf;
		std::vector<std::shared_ptr< std::pair< std::unique_ptr<uint8_t[]>, size_t > const> > m_out_messages;
		std::shared_ptr<ZstTCPGraphTransport> m_transport;
		uuid m_origin_endpoint_UUID;
	};

}