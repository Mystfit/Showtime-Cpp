#include "ZstUDPGraphTransport.h"
#include "ZstSTUNService.h"
#include <showtime/ZstConstants.h>
#include <boost/thread/executor.hpp>
#include <sstream>
#include <regex>

using namespace boost::asio::ip;

namespace showtime 
{
	const std::regex address_match("^([a-z]*?):\/\/([^:^/]*)+?:(\d+)?");

	ZstUDPGraphTransport::ZstUDPGraphTransport(boost::asio::io_context& context) :
		m_recv_buf(),
		m_udp_sock(std::make_shared<boost::asio::ip::udp::socket>(context)),
		m_port(CLIENT_UNRELIABLE_PORT)
	{
		//m_loop_thread = boost::thread(boost::ref(m_ioloop));
		
	}

	ZstUDPGraphTransport::~ZstUDPGraphTransport()
	{
	}

	void ZstUDPGraphTransport::destroy()
	{
		ZstGraphTransport::destroy();

		disconnect();
		//m_ioloop.IO_context().stop();
		//m_loop_thread.interrupt();
		//m_loop_thread.join();
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
				boost::asio::ip::udp::resolver resolver(m_udp_sock->get_executor());
				boost::asio::ip::udp::resolver::query query(addr, port);
				boost::asio::ip::udp::resolver::iterator iter = resolver.resolve(query);
				remote_endpoint = iter->endpoint();
			}

			m_destination_endpoints.push_back(UDPEndpoint{ address, remote_endpoint });
		}
	}

	std::string ZstUDPGraphTransport::getPublicIPAddress(struct STUNServer server) 
	{
		std::string address;
		/*
		// Bind socket that we'll be communicating with
		m_udp_sock->open(udp::v4());
		m_udp_sock->non_blocking(false);
		udp::endpoint local_endpoint = boost::asio::ip::udp::endpoint(udp::v4(), server.local_port);

#ifdef WIN32
		m_udp_sock->set_option(rcv_timeout_option{ 50 });
		m_udp_sock->set_option(rcv_reuseaddr(true));
		m_udp_sock->set_option(rcv_broadcast(true));
#endif
		boost::system::error_code ec;
		m_udp_sock->bind(local_endpoint, ec);
		Log::net(Log::Level::warn, "STUN bind result: {}", ec.message());
		*/
		// Remote Address
		// First resolve the STUN server address
		
		boost::asio::ip::udp::resolver resolver(m_udp_sock->get_executor());
		boost::asio::ip::udp::resolver::query query(server.address, std::to_string(server.port));
		boost::asio::ip::udp::resolver::iterator iter = resolver.resolve(query);
		udp::endpoint remote_endpoint = iter->endpoint();

		// Construct a STUN request
		struct STUNMessageHeader* request = (STUNMessageHeader*)malloc(sizeof(struct STUNMessageHeader));
		request->type = htons(0x0001);
		request->length = htons(0x0000);
		request->cookie = htonl(0x2112A442);

		for (int index = 0; index < 3; index++)
		{
			srand((unsigned int)time(0));
			request->identifier[index] = rand();
		}

		// Send the request
		try {
			m_udp_sock->non_blocking(false);
			m_udp_sock->send_to(boost::asio::buffer(request, sizeof(struct STUNMessageHeader)), remote_endpoint);
		}
		catch (boost::exception const& ex) {
			Log::net(Log::Level::debug, "Failed to send data to STUN server {}", boost::diagnostic_information(ex));
			//m_udp_sock->close();
			free(request);
			return "";
		}

		// Get reply from TURN server
		char reply[1024];
		udp::endpoint sender_endpoint;
		size_t reply_length = 0;
		size_t iters = 10;
		while (reply_length <= 0 && iters > 0) {
			try {
				reply_length = m_udp_sock->receive_from(boost::asio::buffer(reply, 1024), sender_endpoint);
				if (reply_length > 0) {
					break;
				}
			}
			catch (boost::exception const& ex) {
				iters--;
			}
		}
		m_udp_sock->non_blocking(true);

		if (reply_length <= 0) {
			Log::net(Log::Level::debug, "No data returned from STUN server");
			//m_udp_sock->close();
			free(request);
			return "";
		}

		// Parse STUN server reply
		char* pointer = reply;
		struct STUNMessageHeader* response = (struct STUNMessageHeader*)reply;
		if (response->type == htons(0x0101))
		{
			// Check the identifer
			for (int index = 0; index < 3; index++)
			{
				if (request->identifier[index] != response->identifier[index])
				{
					//m_udp_sock->close();
					free(request);
					return "";
				}
			}

			pointer += sizeof(struct STUNMessageHeader);
			while (pointer < reply + reply_length)
			{
				struct STUNAttributeHeader* header = (struct STUNAttributeHeader*)pointer;
				if (header->type == htons(XOR_MAPPED_ADDRESS_TYPE))
				{
					pointer += sizeof(struct STUNAttributeHeader);
					struct STUNXORMappedIPv4Address* xorAddress = (struct STUNXORMappedIPv4Address*)pointer;
					unsigned int numAddress = htonl(xorAddress->address) ^ 0x2112A442;
					address = fmt::format("{}.{}.{}.{}:{}",
						(numAddress >> 24) & 0xFF,
						(numAddress >> 16) & 0xFF,
						(numAddress >> 8) & 0xFF,
						numAddress & 0xFF,
						ntohs(xorAddress->port) ^ 0x2112);

					//m_udp_sock->close();
					free(request);
					return address;
				}

				pointer += (sizeof(struct STUNAttributeHeader) + ntohs(header->length));
			}
		}

		//m_udp_sock->close();
		free(request);
		return address;
	}

	void ZstUDPGraphTransport::set_incoming_port(uint16_t port)
	{
		m_port = port;
	}

	uint16_t ZstUDPGraphTransport::get_incoming_port() {
		return m_port;
	}

	void ZstUDPGraphTransport::listen()
	{
		// Only start receiving messages after we've bound the port
		m_udp_sock->async_receive(boost::asio::buffer(m_recv_buf), boost::bind(
			&ZstUDPGraphTransport::handle_receive,
			this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred)
		);
	}

	int ZstUDPGraphTransport::bind(const std::string& address)
	{
		udp::endpoint local_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::any(), m_port);
		m_udp_sock->bind(local_endpoint);

		return m_port;
	}

	void ZstUDPGraphTransport::disconnect()
	{
		m_udp_sock->close();
	}

	void ZstUDPGraphTransport::disconnect(const std::string& address)
	{
		auto endpoint_it = std::find_if(m_destination_endpoints.begin(), m_destination_endpoints.end(), [address](const UDPEndpoint& endpoint) {return address == endpoint.address; });
		try {
			Log::net(Log::Level::debug, "Disconnecting UDP endpoint {}", address);
			m_destination_endpoints.erase(endpoint_it);
		}
		catch (std::out_of_range e) {
			Log::net(Log::Level::warn, "Couldn't disconnect UDP endpoint {}", address);
		}
			//throw(std::runtime_error("disconnect_from_client(): Not implemented"));
	}

	void ZstUDPGraphTransport::init_graph_sockets()
	{
		// Socket options
		m_udp_sock->open(udp::v4());
		m_udp_sock->non_blocking(true);

#ifdef WIN32
		m_udp_sock->set_option(rcv_timeout_option{ 50 });
		m_udp_sock->set_option(rcv_reuseaddr(true));
		m_udp_sock->set_option(rcv_broadcast(true));
#endif

		std::stringstream addr;
		addr << "udp://" << ZstSTUNService::local_ip() << ":" << m_port;
		set_graph_addresses(addr.str(), "");
	}

	void ZstUDPGraphTransport::send_message_impl(const uint8_t* msg_buffer, size_t msg_buffer_size, const ZstTransportArgs& args) const
	{
		// Broadcast message to all connected endpoints
		for (const auto& endpoint : m_destination_endpoints) {
			m_udp_sock->async_send_to(boost::asio::buffer(msg_buffer, msg_buffer_size), endpoint.endpoint, [](const boost::system::error_code& error, std::size_t length) {
			});
		}
	}

	void ZstUDPGraphTransport::handle_send(const boost::system::error_code& error, std::size_t length)
	{
	}

	void ZstUDPGraphTransport::handle_receive(const boost::system::error_code& error, std::size_t length)
	{
		flatbuffers::Verifier verifier((uint8_t*)m_recv_buf.data(), length);
		if (length <= 0 || !VerifyGraphMessageBuffer(verifier)) {
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
			listen();
		});
	}
}