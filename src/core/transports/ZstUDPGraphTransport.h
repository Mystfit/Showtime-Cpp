#pragma once

#include <showtime/ZstExports.h>
#include "ZstGraphTransport.h"
#include <boost/system/error_code.hpp>
#include <boost/array.hpp>
#include <boost/thread.hpp>
#include "../ZstIOLoop.h"

namespace showtime {
	class ZstUDPGraphTransport :
		public ZstGraphTransport
	{
	public:
		struct STUNServer
		{
			std::string address;
			uint16_t port;
			uint16_t local_port;
		};

		struct UDPEndpoint {
			std::string address;
			boost::asio::ip::udp::endpoint endpoint;
		};

		ZST_EXPORT ZstUDPGraphTransport(boost::asio::io_context& context);
		ZST_EXPORT ~ZstUDPGraphTransport();
		ZST_EXPORT virtual void destroy() override;
		ZST_EXPORT virtual void connect(const std::string& address) override;
		ZST_EXPORT std::string getPublicIPAddress(STUNServer server);
		ZST_EXPORT void set_incoming_port(uint16_t port);
		ZST_EXPORT uint16_t get_incoming_port();
		ZST_EXPORT void start_listening();

		ZST_EXPORT virtual int bind(const std::string& address) override;
		ZST_EXPORT virtual void disconnect() override;
		ZST_EXPORT virtual void disconnect(const std::string& address) override;

	protected:
		ZST_EXPORT virtual void init_graph_sockets() override;
		ZST_EXPORT virtual void send_message_impl(const uint8_t* msg_buffer, size_t msg_buffer_size, const ZstTransportArgs& args) const override;

	private:
		void handle_send(const boost::system::error_code& error, std::size_t);
		void handle_receive(const boost::system::error_code& error, std::size_t);
		
		boost::array<char, 1024> m_recv_buf;
		std::shared_ptr<boost::asio::ip::udp::socket> m_udp_sock;
		uint16_t m_port;

		std::vector<UDPEndpoint> m_destination_endpoints;

		//boost::thread m_loop_thread;
		//ZstIOLoop m_ioloop;
		ZstIOLoop m_handshakecontext;

	};
}