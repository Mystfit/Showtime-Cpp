#include "ZstTCPGraphTransport.h"
#include <czmq.h>
#include <sstream>

ZstTCPGraphTransport::ZstTCPGraphTransport()
{
}

ZstTCPGraphTransport::~ZstTCPGraphTransport()
{
    destroy_graph_sockets();
}

void ZstTCPGraphTransport::connect(const std::string& address)
{
	if (input_graph_socket()) {
		ZstLog::net(LogLevel::notification, "Connecting to {}", address);
		zsock_connect(input_graph_socket(), "%s", address.c_str());
		zsock_set_subscribe(input_graph_socket(), "");
	}
}

void ZstTCPGraphTransport::bind(const std::string& address)
{
	throw(std::runtime_error("bind(): Not implemented"));
}

void ZstTCPGraphTransport::disconnect()
{
	throw(std::runtime_error("disconnect_from_client(): Not implemented"));
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

	set_graph_addresses("", zsock_last_endpoint(graph_out));
	attach_graph_sockets(graph_in, graph_out);
}
