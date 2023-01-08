#include "ZstTCPGraphTransport.h"
#include <sstream>
#include <czmq.h>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/strand.hpp>
#include <showtime/ZstLogging.h>
#include "ZstTCPSession.h"
#include "ZstSTUNService.h"

using namespace boost::asio::ip;

namespace showtime {

ZstTCPGraphTransport::ZstTCPGraphTransport(boost::asio::io_context& context) :
	m_ctx(context)
{
}

ZstTCPGraphTransport::~ZstTCPGraphTransport()
{
    destroy_graph_sockets();
}

int ZstTCPGraphTransport::bind(const std::string& address)
{
	boost::system::error_code ec;

	tcp::endpoint local_endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::any(), get_port());
	m_tcp_sock->bind(local_endpoint, ec);
	if (ec) {
		Log::net(Log::Level::error, "TCP transport bind error: {}", ec.message());
		return -1;
	}

	if (!get_port()) {
		m_port = m_tcp_sock->local_endpoint().port();
		Log::net(Log::Level::notification, "TCP transport OS assigned port: {}", m_port);
	}

	// Make sure addresses are up to date
	std::stringstream addr;
	addr << ZstSTUNService::local_ip() << ":" << m_tcp_sock->local_endpoint().port();
	set_graph_addresses("", addr.str());
	
	return m_port;
}

void ZstTCPGraphTransport::listen()
{
	// Start listening for connections
	boost::system::error_code ec;

	if (ec) {
		Log::server(Log::Level::error, "TCP transport error on open: {}", ec.message());
		return;
	}
	
	if (m_tcp_sock) {
		if(!m_tcp_sock->is_open()) {
			// Recreate socket if it's been closed already
			m_tcp_sock = std::make_shared<tcp::socket>(m_tcp_sock->get_executor());
		}
	}

	// Make sure socket is bound and being used by the acceptor
	//bind("");
	m_acceptor = std::make_shared<tcp::acceptor>(m_ctx, boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::any(), get_port()), true);

	if (ec) {
		Log::server(Log::Level::error, "TCP transport error on acceptor creation: {}", ec.message());
		return;
	}

	m_acceptor->listen(boost::asio::socket_base::max_listen_connections, ec);
	if (ec) {
		Log::server(Log::Level::error, "TCP transport error on listen: {}", ec.message());
		return;
	}

	// The new connection is handled by the network executor
	auto handler = boost::bind(
		&ZstTCPGraphTransport::handle_accept,
		ZstTransportLayer::downcasted_shared_from_this<ZstTCPGraphTransport>(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::detail::placeholder<2>::get()
	);
	m_acceptor->async_accept(
		m_ctx,
		handler
	);
}


void ZstTCPGraphTransport::connect(const std::string& address)
{
	std::smatch base_match;
	std::regex_search(address, base_match, address_match);
	std::string addr = base_match[1].str();
	std::string port = base_match.suffix();

	if (m_tcp_sock) {
		Log::net(Log::Level::notification, "Remembering endpoint peer {}", address);
		//zsock_connect(output_graph_socket(), "%s", address.c_str());

		// Resolve destination endpoint
		tcp::endpoint remote_endpoint;
		try {
			boost::asio::ip::address ip_address = boost::asio::ip::address::from_string(addr);
			remote_endpoint = tcp::endpoint(ip_address, std::stoi(port));
		}
		catch (std::exception e) {
			Log::net(Log::Level::debug, "Address {} needs to be resolved first {}", addr, e.what());
			boost::asio::ip::tcp::resolver resolver(m_tcp_sock->get_executor());
			boost::asio::ip::tcp::resolver::query query(addr, port);
			boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);
			remote_endpoint = iter->endpoint();
		}

		boost::system::error_code ec;
		m_tcp_sock->connect(remote_endpoint, ec);
	}
}

void ZstTCPGraphTransport::disconnect()
{
	boost::system::error_code ec;
	for (auto session : m_graph_connections) {
		session.second->disc
	}
	m_tcp_sock->close(ec);
	if (ec) {
		Log::net(Log::Level::error, "TCP transport error on disconnect: {}", ec.message());
	}
}

std::string ZstTCPGraphTransport::getPublicIPAddress(struct STUNServer server)
{
	auto orig_endpoint = m_tcp_sock->local_endpoint();
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

	boost::system::error_code ec;
	m_tcp_sock->shutdown(boost::asio::socket_base::shutdown_type::shutdown_both);
	m_tcp_sock->close();

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
			if (header->type == htons(STUN_ATTR_XOR_MAPPED_ADDRESS))
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

bool ZstTCPGraphTransport::is_connected_to(const ZstURI& client)
{
	return m_graph_connections.find(client) != m_graph_connections.end();
}

void ZstTCPGraphTransport::init_graph_sockets()
{
	boost::system::error_code ec;

	//auto endpoint = tcp::endpoint(boost::asio::ip::address_v4::any(), 0);

	// Open the acceptor
	//m_tcp_sock->open(endpoint.protocol(), ec);
	//if (ec) {
	//	Log::server(Log::Level::error, "TCP transport error on open: {}", ec.message());
	//	return;
	//}

	// Allow address reuse
/*	m_tcp_sock->set_option(boost::asio::socket_base::reuse_address(true), ec);
	if (ec) {
		Log::server(Log::Level::error, "TCP transport error on set_option reuse_address: {}", ec.message());
		
	}*/
}

//void ZstTCPGraphTransport::handle_accept(const boost::system::error_code& error, tcp::socket& socket)
//{
//	if (error)
//	{
//		Log::net(Log::Level::error, "Error when accepting TCP connection {}", error.message());
//	}
//	else
//	{
//		Log::net(Log::Level::debug, "TCP socket received new connection from {}", socket.remote_endpoint().address().to_string());
//
//		// Create the session and run it
//		auto transport = ZstTransportLayer::downcasted_shared_from_this<ZstTCPGraphTransport>();
//		auto connection = std::make_shared<ZstTCPSession>(std::move(socket), transport);
//		connection->listen();
//		
//		// Store the pending TCP connection 
//		m_pending_graph_connections.push_back(connection);
//	}
//
//	// Accept another connection
//	listen();
//}

void ZstTCPGraphTransport::send_message_impl(std::shared_ptr<flatbuffers::FlatBufferBuilder> buffer_builder, const ZstTransportArgs& args) const
{
	// Send message to all connected clients
	for (auto connection : m_graph_connections) {
		connection.second->queue_write(buffer_builder);
	}
}

}
