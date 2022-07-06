#include "ZstUDPGraphTransport.h"
#include "ZstSTUNService.h"
#include <showtime/ZstConstants.h>
#include <sstream>
#include <regex>

using namespace boost::asio::ip;

namespace showtime 
{
	const std::regex address_match("^([a-z]*?):\/\/([^:^/]*)+?:(\d+)?");

	ZstUDPGraphTransport::ZstUDPGraphTransport() : 
		m_port(CLIENT_UNRELIABLE_PORT)
	{
		m_loop_thread = boost::thread(boost::ref(m_ioloop));
		m_udp_sock = std::make_shared<boost::asio::ip::udp::socket>(m_ioloop.IO_context());
	}

	ZstUDPGraphTransport::~ZstUDPGraphTransport()
	{
	}

	void ZstUDPGraphTransport::destroy()
	{
		ZstGraphTransport::destroy();

		m_udp_sock->close();

		m_ioloop.IO_context().stop();
		m_loop_thread.interrupt();
		m_loop_thread.join();
	}

	void ZstUDPGraphTransport::connect(const std::string& address)
	{
		std::smatch base_match;
		std::regex_search(address, base_match, address_match);
		std::string proto = base_match[1].str();
		std::string addr = base_match[2].str();
		std::string port = base_match.suffix();

		if (m_udp_sock) {
			Log::net(Log::Level::notification, "Remembering endpoint peer {}", address);
			//zsock_connect(output_graph_socket(), "%s", address.c_str());

			// Resolve destination endpoint
			udp::endpoint remote_endpoint;
			try {
				boost::asio::ip::address ip_address = boost::asio::ip::address::from_string(addr);
				remote_endpoint = udp::endpoint(ip_address, std::stoi(port));
			}
			catch (std::exception e) {
				Log::net(Log::Level::debug, "Address {} needs to be resolved first {}", addr, e.what());
				boost::asio::ip::udp::resolver resolver(m_ioloop.IO_context());
				boost::asio::ip::udp::resolver::query query(addr, port);
				boost::asio::ip::udp::resolver::iterator iter = resolver.resolve(query);
				remote_endpoint = iter->endpoint();
			}

			m_destination_endpoints.push_back(remote_endpoint);
		}
	}

	void ZstUDPGraphTransport::set_incoming_port(uint16_t port)
	{
		m_port = port;
	}

	uint16_t ZstUDPGraphTransport::get_incoming_port() {
		return m_port;
	}

	int ZstUDPGraphTransport::bind(const std::string& address)
	{
		udp::endpoint local_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::any(), m_port);
		m_udp_sock->bind(local_endpoint);
		return m_port;
	}

	void ZstUDPGraphTransport::disconnect()
	{
		//throw(std::runtime_error("disconnect_from_client(): Not implemented"));
	}

	void ZstUDPGraphTransport::init_graph_sockets()
	{
		// Socket options
		m_udp_sock->open(udp::v4());
		m_udp_sock->non_blocking(true);

		m_udp_sock->async_receive(boost::asio::buffer(m_recv_buf), boost::bind(
			&ZstUDPGraphTransport::handle_receive,
			this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred)
		);

		std::stringstream addr;
		addr << "udp://" << ZstSTUNService::local_ip() << ":" << m_port;
		set_graph_addresses(addr.str(), "");
	}

	void ZstUDPGraphTransport::send_message_impl(const uint8_t* msg_buffer, size_t msg_buffer_size, const ZstTransportArgs& args) const
	{
		// Broadcast message to all connected endpoints
		for (const auto& endpoint : m_destination_endpoints) {
			m_udp_sock->async_send_to(boost::asio::buffer(msg_buffer, msg_buffer_size), endpoint, [this, endpoint](const boost::system::error_code& error, std::size_t length) {
			});
		}
	}

	void ZstUDPGraphTransport::handle_send(const boost::system::error_code& error, std::size_t length)
	{
	}

	void ZstUDPGraphTransport::handle_receive(const boost::system::error_code& error, std::size_t length)
	{
		if (length <= 0) {
			// Go back to listening
			m_udp_sock->async_receive(boost::asio::buffer(m_recv_buf), boost::bind(
				&ZstUDPGraphTransport::handle_receive,
				this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred)
			);
			return;
		}

		// Link contents of the receive buffer into a message
		auto perf_msg = this->get_msg();
		auto owner = std::static_pointer_cast<ZstGraphTransport>(ZstTransportLayer::shared_from_this());
		perf_msg->init(GetGraphMessage(m_recv_buf.data()), owner);

		//Publish message to other modules
		dispatch_receive_event(perf_msg, [perf_msg, this](ZstEventStatus s) mutable {
			// Cleanup
			this->release(perf_msg);

			// Listen for more messages again
			m_udp_sock->async_receive(boost::asio::buffer(m_recv_buf), boost::bind(
				&ZstUDPGraphTransport::handle_receive,
				this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred)
			);
		});

	}
}