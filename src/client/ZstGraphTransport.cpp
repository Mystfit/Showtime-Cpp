#include "ZstGraphTransport.h"

ZstGraphTransport::ZstGraphTransport()
{
}


ZstGraphTransport::~ZstGraphTransport()
{
}

void ZstGraphTransport::init(ZstActor * actor)
{
	ZstTransportLayerBase::init(actor);

	m_graph_in = zsock_new(ZMQ_SUB);
	if (m_graph_in) {
		zsock_set_linger(m_graph_in, 0);
		zsock_set_unbounded(m_graph_in);
		this->actor()->attach_pipe_listener(m_graph_in, s_handle_graph_in, this);
	}

	std::string network_ip = first_available_ext_ip();
	ZstLog::net(LogLevel::notification, "Using external IP {}", network_ip);

	//Graph output socket
	std::stringstream addr;
	addr << "@tcp://" << network_ip.c_str() << ":*";
	m_graph_out = zsock_new_pub(addr.str().c_str());
	m_graph_out_addr = zsock_last_endpoint(m_graph_out);
	addr.str("");

	//Set graph socket as unbounded by HWM
	zsock_set_unbounded(m_graph_out);
	char * output_ip = zsock_last_endpoint(m_graph_out);
	m_graph_out_ip = std::string(output_ip);
	zstr_free(&output_ip);
	ZstLog::net(LogLevel::notification, "Client graph address: {}", m_graph_out_ip);

	if (m_graph_out)
		zsock_set_linger(m_graph_out, 0);
}

void ZstGraphTransport::destroy()
{
	zsock_destroy(&m_graph_in);
	zsock_destroy(&m_graph_out);
}

void ZstGraphTransport::connect_to_client(const char * endpoint_ip)
{
	ZstLog::net(LogLevel::notification, "Connecting to {}", endpoint_ip);

	//Connect to endpoint publisher
	zsock_connect(m_graph_in, "%s", endpoint_ip);
	zsock_set_subscribe(m_graph_in, "");
}

void ZstGraphTransport::disconnect_from_client()
{
	throw(std::runtime_error("disconnect_from_client(): Not implemented"));
}

const std::string & ZstGraphTransport::get_graph_address() const
{
	return m_graph_out_addr;
}

void ZstGraphTransport::send_message_impl(ZstMessage * msg)
{
	ZstTransportLayer<ZstPerformanceMessage>::send_sock_msg(m_graph_out, static_cast<ZstPerformanceMessage*>(msg));
}

int ZstGraphTransport::s_handle_graph_in(zloop_t * loop, zsock_t * sock, void * arg)
{
	ZstGraphTransport * transport = (ZstGraphTransport*)arg;

	//Forward performance message to adaptors to process
	transport->graph_recv(transport->sock_recv(transport->m_graph_in, false));
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

std::string ZstGraphTransport::first_available_ext_ip()
{
	ziflist_t * interfaces = ziflist_new();
	std::string interface_ip_str = "127.0.0.1";

	if (ziflist_first(interfaces) != NULL) {
		interface_ip_str = std::string(ziflist_address(interfaces));
	}

	return interface_ip_str;
}
