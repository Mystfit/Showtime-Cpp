#include <sstream>
#include <czmq.h>
#include <showtime/ZstLogging.h>
#include "ZstTCPGraphTransport.h"
#include "ZstTCPSession.h"
#include "ZstSTUNService.h"

using namespace boost::asio::ip;

namespace showtime {

ZstTCPGraphTransport::ZstTCPGraphTransport(boost::asio::io_context& context) :
	m_tcp_sock(std::make_shared<tcp::socket>(context))
{
	// We keep the TCP socket seperate to the acceptor so we can use it to talk to the STUN server
	m_acceptor = std::make_shared<tcp::acceptor>(context, m_tcp_sock);
}

ZstTCPGraphTransport::~ZstTCPGraphTransport()
{
    destroy_graph_sockets();
	m_acceptor->close();
	m_tcp_sock->close();
}

int ZstTCPGraphTransport::bind(const std::string& address)
{
	tcp::endpoint local_endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::any(), m_port);
	m_tcp_sock->bind(local_endpoint);
}

void ZstTCPGraphTransport::listen()
{
	// The new connection gets its own strand
	m_acceptor->async_accept(
		boost::asio::make_strand(m_tcp_sock->get_executor()),
		boost::bind(
			&ZstTCPGraphTransport::handle_accept, 
			this, 
			boost::asio::placeholders::error, 
			boost::asio::placeholders::detail::placeholder<2>::get()
		)
	);
}


void ZstTCPGraphTransport::connect(const std::string& address)
{
	if (!input_graph_socket())
		return;

}

void ZstTCPGraphTransport::disconnect()
{

}

std::string ZstTCPGraphTransport::getPublicIPAddress(struct STUNServer server)
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

	boost::asio::ip::tcp::resolver resolver(m_tcp_sock->get_executor());
	boost::asio::ip::tcp::resolver::query query(server.address, std::to_string(server.port));
	boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);
	tcp::endpoint remote_endpoint = iter->endpoint();

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
		m_tcp_sock->non_blocking(false);
		m_tcp_sock->connect(remote_endpoint);
		m_tcp_sock->send(boost::asio::buffer(request, sizeof(struct STUNMessageHeader)));
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
			reply_length = m_tcp_sock->receive(boost::asio::buffer(reply, 1024));
			if (reply_length > 0) {
				break;
			}
		}
		catch (boost::exception const& ex) {
			iters--;
		}
	}
	m_tcp_sock->non_blocking(true);

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

void ZstTCPGraphTransport::init_graph_sockets()
{
	boost::system::error_code ec;

	auto endpoint = tcp::endpoint(boost::asio::ip::address_v4::any(), 0);

	// Open the acceptor
	m_tcp_sock->open(endpoint.protocol(), ec);
	if (ec) {
		Log::server(Log::Level::error, "TCP transport error on open: {}", ec.message());
		return;
	}

	// Allow address reuse
	m_tcp_sock->set_option(boost::asio::socket_base::reuse_address(true), ec);
	if (ec) {
		Log::server(Log::Level::error, "TCP transport error on set_option reuse_address: {}", ec.message());
		return;
	}

	// Start listening for connections
	m_acceptor->listen(boost::asio::socket_base::max_listen_connections, ec);
	if (ec) {
		Log::server(Log::Level::error, "TCP transport error on listen: {}", ec.message());
		return;
	}

	Log::server(Log::Level::debug, "TCP transport listening on port {}", endpoint.port());
}

void ZstTCPGraphTransport::handle_accept(const boost::system::error_code& error, tcp::socket socket)
{
	if (error)
	{
		Log::net(Log::Level::error, "Error when accepting TCP connection {}", error.message());
	}
	else
	{
		Log::net(Log::Level::debug, "TCP socket received new connection from {}", socket.remote_endpoint().address().to_string());

		// Create the session and run it
		auto connection = std::make_shared<ZstTCPSession>(std::move(socket), ZstTransportLayer::downcasted_shared_from_this<ZstTCPGraphTransport>());
		connection->run();
		m_connections.insert(std::pair<uuid, std::shared_ptr<ZstTCPSession>>(connection->origin_endpoint_UUID(), connection));
	}

	// Accept another connection
	listen();
}

}
