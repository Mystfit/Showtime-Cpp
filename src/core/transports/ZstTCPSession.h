#pragma once

#include <memory>
#include <string>
#include <boost/array.hpp>
#include <boost/uuid/uuid.hpp>
#include <concurrentqueue.h>
#include "ZstTCPGraphTransport.h"

using namespace boost::uuids;

namespace showtime {

	class ZstTCPSession : public std::enable_shared_from_this<ZstTCPSession> {
	public:
		// Take ownership of the socket and the owning transport
		explicit ZstTCPSession(std::shared_ptr<ZstTCPGraphTransport> transport, boost::asio::io_context& context);
		~ZstTCPSession();
		void listen();

		void do_read();

		void queue_write(std::shared_ptr<flatbuffers::FlatBufferBuilder> buffer_builder);
		void do_write();
		
		void on_read(const boost::system::error_code& error, std::size_t bytes_transferred);
		void on_write(const boost::system::error_code& error, std::size_t bytes_transferred);

		const uuid& origin_endpoint_UUID();
	private:
		boost::asio::io_context& m_ctx;
		std::shared_ptr<boost::asio::ip::tcp::socket> m_socket;

		boost::array<char, 1024*8> m_recv_buf;
		moodycamel::BlockingConcurrentQueue<std::shared_ptr<flatbuffers::FlatBufferBuilder>> m_out_messages;

		std::shared_ptr<ZstTCPGraphTransport> m_transport;
		uuid m_origin_endpoint_UUID;
	};

}