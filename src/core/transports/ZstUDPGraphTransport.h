#pragma once

#include <showtime/ZstExports.h>
#include "ZstGraphTransport.h"
#include <boost/system/error_code.hpp>
#include <boost/array.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/thread.hpp>
#include "../ZstIOLoop.h"

namespace showtime {
	class ZstUDPGraphTransport :
		public ZstGraphTransport
	{
	public:
		struct UDPEndpoint {
			std::string address;
			boost::asio::ip::udp::endpoint endpoint;
		};

		ZST_EXPORT ZstUDPGraphTransport(boost::asio::io_context& context);
		ZST_EXPORT ~ZstUDPGraphTransport();
		ZST_EXPORT virtual void destroy() override;
		ZST_EXPORT virtual void connect(const std::string& address) override;
		ZST_EXPORT virtual std::string getPublicIPAddress(STUNServer server) override;
		ZST_EXPORT virtual void listen() override;

		ZST_EXPORT virtual int bind(const std::string& address) override;
		ZST_EXPORT virtual void disconnect() override;
		ZST_EXPORT virtual void disconnect(const std::string& address) override;

	protected:
		ZST_EXPORT virtual void init_graph_sockets() override;
		ZST_EXPORT virtual void send_message_impl(std::shared_ptr<flatbuffers::FlatBufferBuilder> buffer_builder, const ZstTransportArgs& args) const override;

	private:
		void handle_send(const boost::system::error_code& error, std::size_t);
		void handle_receive(const boost::system::error_code& error, std::size_t);
		
		boost::array<char, 1024> m_recv_buf;
		std::shared_ptr<boost::asio::ip::udp::socket> m_udp_sock;

		std::vector<UDPEndpoint> m_destination_endpoints;
	};
}