#include "ZstUDPGraphTransport.h"
#include <czmq.h>
#include <sstream>

namespace showtime 
{
	ZstUDPGraphTransport::ZstUDPGraphTransport()
	{
	}

	ZstUDPGraphTransport::~ZstUDPGraphTransport()
	{
	}

	void ZstUDPGraphTransport::connect(const std::string& address)
	{
		if (output_graph_socket()) {
			Log::net(Log::Level::notification, "Connecting to {}", address);
			zsock_connect(output_graph_socket(), "%s", address);
		}
	}

	int ZstUDPGraphTransport::bind(const std::string& address)
	{
		//throw(std::runtime_error("bind(): Not implemented"));
		return -1;
	}

	void ZstUDPGraphTransport::disconnect()
	{
		//throw(std::runtime_error("disconnect_from_client(): Not implemented"));
	}

	void ZstUDPGraphTransport::init_graph_sockets()
	{
		//UDP sockets are reversed - graph in needs to bind, graph out connects
		std::stringstream addr;
		std::string protocol = "udp";

		//Output socket
		addr << ">" << protocol << "://" << CLIENT_MULTICAST_ADDR << ":" << CLIENT_UNRELIABLE_PORT;
		zsock_t* graph_out = zsock_new_radio(addr.str().c_str());
		if (!graph_out) {
			Log::net(Log::Level::error, "Could not create UDP output socket. ZMQ returned {}", std::strerror(zmq_errno()));
			return;
		}
		addr.str("");

		//Input socket
		addr << "@" << protocol << "://" << CLIENT_MULTICAST_ADDR << ":" << CLIENT_UNRELIABLE_PORT;
		zsock_t* graph_in = zsock_new_dish(addr.str().c_str());
		if (!graph_in) {
			Log::net(Log::Level::error, "Could not create UDP input socket. ZMQ returned {}", std::strerror(zmq_errno()));
			return;
		}

		zsock_set_linger(graph_out, 0);
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

}