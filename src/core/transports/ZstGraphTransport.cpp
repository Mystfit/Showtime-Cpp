#include "ZstGraphTransport.h"
#include <zmq.h>

ZstGraphTransport::ZstGraphTransport() :
	m_graph_in(NULL),
	m_graph_out(NULL)
{
}

ZstGraphTransport::~ZstGraphTransport()
{
}

void ZstGraphTransport::init()
{
	ZstTransportLayerBase::init();

	m_graph_actor.init("graph");
	init_graph_sockets();
	m_graph_actor.start_loop();
}

void ZstGraphTransport::destroy()
{
	ZstTransportLayerBase::destroy();

	m_graph_actor.stop_loop();
	destroy_graph_sockets();
	m_graph_actor.destroy();
}

const std::string & ZstGraphTransport::get_graph_in_address() const
{
	return m_graph_in_addr;
}

const std::string & ZstGraphTransport::get_graph_out_address() const
{
	return m_graph_out_addr;
}

void ZstGraphTransport::send_message_impl(ZstMessage * msg)
{
	ZstPerformanceMessage * perf_msg = static_cast<ZstPerformanceMessage*>(msg);
	zframe_t * payload_frame = perf_msg->payload_frame();

	zframe_set_group(payload_frame, PERFORMANCE_GROUP);
	zsock_t * sock = output_graph_socket();
	int result = (sock) ? zframe_send(&payload_frame, sock, 0) : -1;
	if (result < 0) {
		ZstLog::net(LogLevel::warn, "Message failed to send with status {}", result);
	}

	m_msg_pool.release(static_cast<ZstPerformanceMessage*>(msg));
}

ZstActor & ZstGraphTransport::actor()
{
	return m_graph_actor;
}

void ZstGraphTransport::attach_graph_sockets(zsock_t * in, zsock_t * out)
{
	m_graph_actor.attach_pipe_listener(in, s_handle_graph_in, this);
	m_graph_in = in;
	m_graph_out = out;
}

void ZstGraphTransport::set_graph_addresses(const std::string & in_addr, const std::string & out_addr)
{
	m_graph_out_addr = out_addr;
	m_graph_in_addr = in_addr;
	ZstLog::net(LogLevel::debug, "Graph transport address in: {} out: {}", m_graph_in_addr, m_graph_out_addr);
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

	if (is_active()) {
		//Publish message to other modules
		msg_events()->invoke([this, perf_msg](ZstTransportAdaptor* adaptor) {
			if (this->is_active())
				adaptor->on_receive_msg(perf_msg);
		});
	}

	release_msg(perf_msg);
}

void ZstGraphTransport::graph_recv(zframe_t * frame)
{
	//Unpack the frame into a message
	ZstPerformanceMessage * perf_msg = get_msg();
	perf_msg->unpack(frame);
	
	//Publish message to other modules
	msg_events()->invoke([this, perf_msg](ZstTransportAdaptor* adaptor) {
		if(this->is_active())
			adaptor->on_receive_msg(perf_msg);
	});

	//Clean up resources once other modules have finished with this message
	release_msg(perf_msg);
	zframe_destroy(&frame);
}

void ZstGraphTransport::destroy_graph_sockets()
{
	if (m_graph_in)
		zsock_destroy(&m_graph_in);
	if (m_graph_out)
		zsock_destroy(&m_graph_out);
}

std::string ZstGraphTransport::first_available_ext_ip() const
{
	ziflist_t * interfaces = ziflist_new();
	std::string interface_ip_str = "127.0.0.1";

	if (ziflist_first(interfaces) != NULL) {
		interface_ip_str = std::string(ziflist_address(interfaces));
	}

	return interface_ip_str;
}

zsock_t * ZstGraphTransport::input_graph_socket()
{
	return m_graph_in;
}

zsock_t * ZstGraphTransport::output_graph_socket()
{
	return m_graph_out;
}