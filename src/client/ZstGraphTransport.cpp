#include "ZstGraphTransport.h"
#include <zmq.h>

ZstGraphTransport::ZstGraphTransport() :
	m_graph_in_reliable(NULL),
	m_graph_out_reliable(NULL),
	m_graph_in_unreliable(NULL),
	m_graph_out_unreliable(NULL),
	m_graph_in_reliable_local(NULL),
	m_graph_out_reliable_local(NULL)
{
}

ZstGraphTransport::~ZstGraphTransport()
{
}

void ZstGraphTransport::init()
{
	ZstTransportLayerBase::init();
	m_reliable_graph_actor.init("graph_reliable");
	m_unreliable_graph_actor.init("graph_unreliable");

	init_remote_graph_sockets();
	//init_local_graph_sockets();
	init_unreliable_graph_sockets();

	m_reliable_graph_actor.start_loop();
	m_unreliable_graph_actor.start_loop();
}

void ZstGraphTransport::destroy()
{
	ZstTransportLayerBase::destroy();

	m_reliable_graph_actor.stop_loop();
	m_unreliable_graph_actor.stop_loop();

	destroy_reliable_graph_sockets();
	destroy_unreliable_graph_sockets();
	destroy_local_graph_sockets();

	m_reliable_graph_actor.destroy();
	m_unreliable_graph_actor.destroy();
}

void ZstGraphTransport::connect_to_reliable_client(const char * reliable_endpoint)
{
	ZstLog::net(LogLevel::notification, "Connecting to {}", reliable_endpoint);
	zsock_connect(m_graph_in_reliable, "%s", reliable_endpoint);
	zsock_set_subscribe(m_graph_in_reliable, "");
}

void ZstGraphTransport::connect_to_unreliable_client(const char * unreliable_endpoint)
{
	if (m_graph_out_unreliable) {
		ZstLog::net(LogLevel::notification, "Connecting to {}", unreliable_endpoint);
		zsock_connect(m_graph_out_unreliable, "%s", unreliable_endpoint);
	}
}

void ZstGraphTransport::disconnect_from_client()
{
	throw(std::runtime_error("disconnect_from_client(): Not implemented"));
}

const std::string & ZstGraphTransport::get_reliable_graph_address() const
{
	return m_graph_out_reliable_addr;
}

const std::string & ZstGraphTransport::get_unreliable_graph_address() const
{
	return m_graph_in_unreliable_addr;
}

void ZstGraphTransport::send_message_impl(ZstMessage * msg)
{
	ZstPerformanceMessage * perf_msg = static_cast<ZstPerformanceMessage*>(msg);
	zframe_t * payload_frame = perf_msg->payload_frame();
	zsock_t * socket = (msg->has_arg(ZstMsgArg::UNRELIABLE)) ? m_graph_out_unreliable : m_graph_out_reliable;

	zframe_set_group(payload_frame, PERFORMANCE_GROUP);
	int result = (socket) ? zframe_send(&payload_frame, socket, 0) : -1;
	if (result < 0) {
		ZstLog::net(LogLevel::warn, "Message failed to send with status {}", result);
	}
		
	m_msg_pool.release(static_cast<ZstPerformanceMessage*>(msg));
}

int ZstGraphTransport::s_handle_graph_in(zloop_t * loop, zsock_t * sock, void * arg)
{
	((ZstGraphTransport*)arg)->graph_recv(zframe_recv(sock));
	return 0;
}

void ZstGraphTransport::graph_recv(zmsg_t * msg)
{
	ZstPerformanceMessage * perf_msg = get_msg();
	perf_msg->unpack(msg);
	zmsg_destroy(&msg);

	//Publish message to other modules
	msg_events()->invoke([perf_msg](ZstTransportAdaptor* adaptor) {
		adaptor->on_receive_msg(perf_msg);
	});

	release_msg(perf_msg);
}

void ZstGraphTransport::graph_recv(zframe_t * frame)
{
	//Unpack the frame into a message
	ZstPerformanceMessage * perf_msg = get_msg();
	perf_msg->unpack(frame);
	
	//Publish message to other modules
	msg_events()->invoke([perf_msg](ZstTransportAdaptor* adaptor) {
		adaptor->on_receive_msg(perf_msg);
	});

	//Clean up resources once other modules have finished with this message
	release_msg(perf_msg);
	zframe_destroy(&frame);
}

void ZstGraphTransport::init_remote_graph_sockets()
{
	//In
	m_graph_in_reliable = zsock_new(ZMQ_SUB);
	zsock_set_subscribe(m_graph_in_reliable, "");
	zsock_set_linger(m_graph_in_reliable, 0);
	zsock_set_unbounded(m_graph_in_reliable);
	m_reliable_graph_actor.attach_pipe_listener(m_graph_in_reliable, s_handle_graph_in, this);
	
	//Out
	m_graph_out_reliable = zsock_new(ZMQ_PUB);
	std::stringstream addr;
	addr << "tcp://" << first_available_ext_ip().c_str() << ":*";
	zsock_set_unbounded(m_graph_out_reliable);
	zsock_set_linger(m_graph_out_reliable, 0);
	int port = zsock_bind(m_graph_out_reliable, addr.str().c_str());
	ZstLog::net(LogLevel::debug, "Bound port: {}", port);
	m_graph_out_reliable_addr = zsock_last_endpoint(m_graph_out_reliable);
	ZstLog::net(LogLevel::notification, "Reliable remote graph using address {}", m_graph_out_reliable_addr);
}

void ZstGraphTransport::init_local_graph_sockets()
{
	/*m_graph_in_reliable_local = zsock_new(ZMQ_SUB);
	m_graph_out_reliable_local = zsock_new(ZMQ_PUB);
	std::stringstream addr;
	addr << "inproc://" << actor()->name();
	init_graph_sockets(m_graph_in_reliable_local, m_graph_out_reliable_local, addr.str());
	m_graph_out_reliable_local_addr = zsock_last_endpoint(m_graph_out_reliable_local);
	ZstLog::net(LogLevel::notification, "Reliable local graph using address {}", m_graph_out_reliable_local_addr);*/
}

void ZstGraphTransport::init_unreliable_graph_sockets()
{
	//UDP sockets are reversed - graph in needs to bind, graph out connects
	std::stringstream addr;
	std::string protocol = "udp";

	//Output socket
	addr << ">" << protocol << "://" << CLIENT_MULTICAST_ADDR << ":" << CLIENT_UNRELIABLE_PORT;
	m_graph_out_unreliable = zsock_new_radio(addr.str().c_str());
	zsock_set_linger(m_graph_out_unreliable, 0);
	addr.str("");

	//Input socket
	addr << "@" << protocol << "://" << CLIENT_MULTICAST_ADDR << ":" << CLIENT_UNRELIABLE_PORT;
	m_graph_in_unreliable = zsock_new_dish(addr.str().c_str());

	zsock_set_linger(m_graph_in_unreliable, 0);
	m_unreliable_graph_actor.attach_pipe_listener(m_graph_in_unreliable, s_handle_graph_in, this);
	zsock_set_rcvbuf(m_graph_in_unreliable, 25000000);

	//Build remote IP
	addr.str("");
	addr << protocol << "://" << first_available_ext_ip() << ":" << CLIENT_UNRELIABLE_PORT;
	m_graph_in_unreliable_addr =  addr.str();
	ZstLog::net(LogLevel::notification, "Unreliable graph using address {}", m_graph_in_unreliable_addr);

	//Join groups
	zsock_join(m_graph_in_unreliable, PERFORMANCE_GROUP);
}

void ZstGraphTransport::destroy_reliable_graph_sockets()
{
	if (m_graph_in_reliable)
		zsock_destroy(&m_graph_in_reliable);
	if (m_graph_out_reliable)
		zsock_destroy(&m_graph_out_reliable);
}

void ZstGraphTransport::destroy_unreliable_graph_sockets()
{
	if (m_graph_in_unreliable)
		zsock_destroy(&m_graph_in_unreliable);
	if (m_graph_out_unreliable)
		zsock_destroy(&m_graph_out_unreliable);
}

void ZstGraphTransport::destroy_local_graph_sockets()
{
	if (m_graph_in_reliable_local)
		zsock_destroy(&m_graph_in_reliable_local);
	if (m_graph_out_reliable_local)
		zsock_destroy(&m_graph_out_reliable_local);
}

std::string ZstGraphTransport::first_available_ext_ip()
{
	ziflist_t * interfaces = ziflist_new();
	std::string interface_ip_str = "127.0.0.1";

	if (ziflist_first(interfaces) != NULL) {
		interface_ip_str = std::string(ziflist_address(interfaces));
	}

	return interface_ip_str;
}
