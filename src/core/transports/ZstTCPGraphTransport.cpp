#include <sstream>
#include <czmq.h>
#include <showtime/ZstLogging.h>
#include "ZstTCPGraphTransport.h"
#include "ZstSTUNService.h"

using namespace boost::asio::ip;

namespace showtime {

ZstTCPGraphTransport::ZstTCPGraphTransport(boost::asio::io_context& context) :
	m_tcp_sock(std::make_shared<boost::asio::ip::tcp::socket>(context))
{
}

ZstTCPGraphTransport::~ZstTCPGraphTransport()
{
    destroy_graph_sockets();
}

void ZstTCPGraphTransport::connect(const std::string& address)
{
	if (!input_graph_socket())
		return;

	if (std::find(m_connected_addresses.begin(), m_connected_addresses.end(), address) != m_connected_addresses.end()){
		Log::net(Log::Level::warn, "Already connected to {}", address);
		return;
	}

	if (input_graph_socket()) {
		Log::net(Log::Level::notification, "Connecting to {}", address);
		
		auto result = zsock_connect(input_graph_socket(), "%s", address.c_str());
		if (result == 0) {
			set_connected(true);
			m_connected_addresses.insert(address);
		}
		else {
			Log::net(Log::Level::error, "Client graph connection error: {}", zmq_strerror(result));
		}

		zsock_set_subscribe(input_graph_socket(), "");
	}
}

void ZstTCPGraphTransport::disconnect()
{
	for (auto address : m_connected_addresses)
		zsock_disconnect(input_graph_socket(), "%s", address.c_str());

	m_connected_addresses.clear();
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
	//In
	zsock_t * graph_in = zsock_new(ZMQ_SUB);
	zsock_set_subscribe(graph_in, "");
	zsock_set_linger(graph_in, 0);
	zsock_set_unbounded(graph_in);

	//Out
	zsock_t *  graph_out = zsock_new(ZMQ_PUB);
	std::stringstream addr;
	addr << "tcp://" << ZstSTUNService::local_ip().c_str() << ":*";
	zsock_set_unbounded(graph_out);
	zsock_set_linger(graph_out, 0);
	int port = zsock_bind(graph_out, "%s", addr.str().c_str());
	Log::net(Log::Level::debug, "Bound port: {}", port);

	auto last_endpoint = zsock_last_endpoint(graph_out);
	set_graph_addresses("", last_endpoint);
	zstr_free(&last_endpoint);

	attach_graph_sockets(graph_in, graph_out);
}

}
