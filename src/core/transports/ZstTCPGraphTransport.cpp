#include <sstream>
#include <czmq.h>
#include "ZstLogging.h"
#include "ZstTCPGraphTransport.h"

namespace showtime {

ZstTCPGraphTransport::ZstTCPGraphTransport()
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
		ZstLog::net(LogLevel::warn, "Already connected to {}", address);
		return;
	}

	if (input_graph_socket()) {
		ZstLog::net(LogLevel::notification, "Connecting to {}", address);
		
		auto result = zsock_connect(input_graph_socket(), "%s", address.c_str());
		if (result == 0) {
			set_connected(true);
			m_connected_addresses.insert(address);
		}
		else {
			ZstLog::net(LogLevel::error, "Client graph connection error: {}", zmq_strerror(result));
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
	addr << "tcp://" << first_available_ext_ip().c_str() << ":*";
	zsock_set_unbounded(graph_out);
	zsock_set_linger(graph_out, 0);
	int port = zsock_bind(graph_out, "%s", addr.str().c_str());
	ZstLog::net(LogLevel::debug, "Bound port: {}", port);

	auto last_endpoint = zsock_last_endpoint(graph_out);
	set_graph_addresses("", last_endpoint);
	zstr_free(&last_endpoint);

	attach_graph_sockets(graph_in, graph_out);
}

}
