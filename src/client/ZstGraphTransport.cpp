#include "ZstGraphTransport.h"

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

void ZstGraphTransport::init(ZstActor * actor)
{
	ZstTransportLayerBase::init(actor);

	init_remote_graph_sockets();
	init_local_graph_sockets();
	init_unreliable_graph_sockets();
}

void ZstGraphTransport::destroy()
{
	ZstTransportLayerBase::destroy();

	destroy_reliable_graph_sockets();
	destroy_unreliable_graph_sockets();
	destroy_local_graph_sockets();
}

void ZstGraphTransport::connect_to_reliable_client(const char * reliable_endpoint)
{
	ZstLog::net(LogLevel::notification, "Connecting to {}", reliable_endpoint);
	zsock_connect(m_graph_in_reliable, "%s", reliable_endpoint);
	zsock_set_subscribe(m_graph_in_reliable, "");
}

void ZstGraphTransport::connect_to_unreliable_client(const char * unreliable_endpoint)
{
	ZstLog::net(LogLevel::notification, "Connecting to {}", unreliable_endpoint);
	zsock_connect(m_graph_out_unreliable, "%s", unreliable_endpoint);
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

	if (msg->has_arg(ZstMsgArg::UNRELIABLE)) {
		//Message is unreliable - send over UDP graph
		zframe_set_group(payload_frame, PERFORMANCE_GROUP);
		int result = zframe_send(&payload_frame, m_graph_out_unreliable, 0);
		if (result < 0) {
			ZstLog::net(LogLevel::warn, "Unreliable message failed to send with status {}", result);
		}
	}
	else {
		zframe_send(&payload_frame, m_graph_out_reliable, 0);
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
	m_graph_in_reliable = zsock_new(ZMQ_SUB);
	m_graph_out_reliable = zsock_new(ZMQ_PUB);
	std::stringstream addr;
	addr << "tcp://" << first_available_ext_ip().c_str() << ":*";
	init_graph_sockets(m_graph_in_reliable, m_graph_out_reliable, addr.str());
	m_graph_out_reliable_addr = zsock_last_endpoint(m_graph_out_reliable);
	ZstLog::net(LogLevel::notification, "Reliable remote graph using address {}", m_graph_out_reliable_addr);
}

void ZstGraphTransport::init_local_graph_sockets()
{
	m_graph_in_reliable_local = zsock_new(ZMQ_SUB);
	m_graph_out_reliable_local = zsock_new(ZMQ_PUB);
	std::stringstream addr;
	addr << "inproc://" << actor()->name();
	init_graph_sockets(m_graph_in_reliable_local, m_graph_out_reliable_local, addr.str());
	m_graph_out_reliable_local_addr = zsock_last_endpoint(m_graph_out_reliable_local);
	ZstLog::net(LogLevel::notification, "Reliable local graph using address {}", m_graph_out_reliable_local_addr);
}

void ZstGraphTransport::init_unreliable_graph_sockets()
{
	m_graph_out_unreliable = zsock_new(ZMQ_RADIO);
	m_graph_in_unreliable = zsock_new(ZMQ_DISH);
	std::stringstream addr;
	std::string protocol = "udp";
	addr << protocol << "://*:" << CLIENT_UNRELIABLE_PORT;

	zsock_set_linger(m_graph_in_unreliable, 0);
	zsock_set_sndhwm(m_graph_in_unreliable, HWM);
	zsock_set_rcvbuf(m_graph_in_unreliable, SOCK_BUFFER);
	this->actor()->attach_pipe_listener(m_graph_in_unreliable, s_handle_graph_in, this);

	//UDP sockets are reversed - graph in needs to bind, graph out connects
	zsock_set_linger(m_graph_out_unreliable, 0);
	zsock_set_sndhwm(m_graph_out_unreliable, HWM);
	zsock_set_rcvbuf(m_graph_out_unreliable, SOCK_BUFFER);
	zsock_bind(m_graph_in_unreliable, addr.str().c_str());

	//Build remote IP
	addr.str("");
	addr << protocol << "://" << first_available_ext_ip() << ":" << CLIENT_UNRELIABLE_PORT;
	m_graph_in_unreliable_addr =  addr.str();
	ZstLog::net(LogLevel::notification, "Unreliable graph using address {}", m_graph_in_unreliable_addr);

	//Join groups
	zsock_join(m_graph_in_unreliable, PERFORMANCE_GROUP);
	//zsock_join(m_graph_in_unreliable, HANDSHAKE_GROUP);
}

void ZstGraphTransport::init_graph_sockets(zsock_t * graph_in, zsock_t * graph_out, const std::string & address)
{
	if (graph_in) {
		zsock_set_linger(graph_in, 0);
		zsock_set_unbounded(graph_in);
		this->actor()->attach_pipe_listener(graph_in, s_handle_graph_in, this);
	}
	
	//Set graph linger to 0 to clean up immediately and unbounded so messages will queue until sent
	if (graph_out) {
		zsock_set_unbounded(graph_out);
		zsock_set_linger(graph_out, 0);
		int port = zsock_bind(graph_out, address.c_str());
		ZstLog::net(LogLevel::debug, "Bound port: {}", port);
	}
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
