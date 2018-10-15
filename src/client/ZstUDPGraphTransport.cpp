#include "ZstUDPGraphTransport.h"

ZstUDPGraphTransport::ZstUDPGraphTransport()
{
}

ZstUDPGraphTransport::~ZstUDPGraphTransport()
{
}

void ZstUDPGraphTransport::connect_to_client(const char * endpoint)
{
	if (output_graph_socket()) {
		ZstLog::net(LogLevel::notification, "Connecting to {}", endpoint);
		zsock_connect(output_graph_socket(), "%s", endpoint);
	}
}

void ZstUDPGraphTransport::disconnect_from_client()
{
	throw(std::runtime_error("disconnect_from_client(): Not implemented"));
}

void ZstUDPGraphTransport::init_graph_sockets()
{
	//UDP sockets are reversed - graph in needs to bind, graph out connects
	std::stringstream addr;
	std::string protocol = "udp";

	//Output socket
	addr << ">" << protocol << "://" << CLIENT_MULTICAST_ADDR << ":" << CLIENT_UNRELIABLE_PORT;
	zsock_t * graph_out = zsock_new_radio(addr.str().c_str());
	zsock_set_linger(graph_out, 0);
	addr.str("");

	//Input socket
	addr << "@" << protocol << "://" << CLIENT_MULTICAST_ADDR << ":" << CLIENT_UNRELIABLE_PORT;
	zsock_t * graph_in = zsock_new_dish(addr.str().c_str());

	zsock_set_linger(graph_in, 0);
	attach_graph_sockets(graph_in, graph_out);
	zsock_set_rcvbuf(graph_in, 25000000);

	//Build remote IP
	addr.str("");
	addr << protocol << "://" << first_available_ext_ip() << ":" << CLIENT_UNRELIABLE_PORT;
	set_graph_addresses(addr.str(), "");

	//Join groups
	zsock_join(graph_in, PERFORMANCE_GROUP);
}
