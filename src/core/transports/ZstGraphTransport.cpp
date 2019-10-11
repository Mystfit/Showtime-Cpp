#include "ZstGraphTransport.h"
#include "../ZstZMQRefCounter.h"
#include <zmq.h>
#include <czmq.h>
#include <msgpack.hpp>

namespace showtime {

ZstGraphTransport::ZstGraphTransport() :
	m_graph_in(NULL),
	m_graph_out(NULL)
{
}

ZstGraphTransport::~ZstGraphTransport()
{
    //m_graph_actor.stop_loop();
    destroy_graph_sockets();
}

void ZstGraphTransport::init()
{
	ZstTransportLayerBase::init();

	m_graph_actor.init("graph");
	init_graph_sockets();
	m_graph_actor.start_loop();
	zst_zmq_inc_ref_count();
}

void ZstGraphTransport::destroy()
{
	m_graph_actor.stop_loop();
	destroy_graph_sockets();
	ZstTransportLayerBase::destroy();
}

const std::string & ZstGraphTransport::get_graph_in_address() const
{
	return m_graph_in_addr;
}

const std::string & ZstGraphTransport::get_graph_out_address() const
{
	return m_graph_out_addr;
}
    
ZstMessageReceipt ZstGraphTransport::send_msg(flatbuffers::Offset<GraphMessage> message_content, flatbuffers::FlatBufferBuilder& buffer_builder, const ZstTransportArgs& args)
{
    flatbuffers::Offset<GraphMessage> msg_offset;
    buffer_builder.Finish(msg_offset);
    send_message_impl(buffer_builder.GetBufferPointer(), buffer_builder.GetSize(), args);
    
    return ZstMessageReceipt{Signal_OK};
}

void ZstGraphTransport::send_message_impl(const uint8_t * msg_buffer, size_t msg_buffer_size, const ZstTransportArgs & args) const
{
	zframe_t * payload_frame = zframe_new(msg_buffer, msg_buffer_size);
#ifdef ZST_BUILD_DRAFT_API
	zframe_set_group(payload_frame, PERFORMANCE_GROUP);
#endif
	zsock_t * sock = output_graph_socket();
	int result = (sock) ? zframe_send(&payload_frame, sock, 0) : -1;
	if (result < 0) {
		ZstLog::net(LogLevel::warn, "Message failed to send with status {}", result);
	}

    delete msg_buffer;
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

void ZstGraphTransport::graph_recv(zframe_t * frame)
{
	//Unpack the frame into a message
	ZstPerformanceMessage * perf_msg = this->get_msg();
    perf_msg->init(GetGraphMessage(zframe_data(frame)));
	
	//Publish message to other modules
	if (this->is_active()) {
		msg_events()->invoke([perf_msg](std::shared_ptr<ZstGraphTransportAdaptor> adaptor) {
			adaptor->on_receive_msg(perf_msg);
		});
	}

	//Clean up resources once other modules have finished with this message
	this->release(perf_msg);
	zframe_destroy(&frame);
}

void ZstGraphTransport::destroy_graph_sockets()
{
    bool deref = false;
    if (m_graph_in){
		//m_graph_actor.remove_pipe_listener(m_graph_in);
		zsock_destroy(&m_graph_in);
		m_graph_in = NULL;
        deref = true;
    }
    if (m_graph_out){
		//m_graph_actor.remove_pipe_listener(m_graph_out);
		zsock_destroy(&m_graph_out);
		m_graph_out = NULL;
        deref = true;
    }
    if(deref)
        zst_zmq_dec_ref_count();
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

zsock_t * ZstGraphTransport::input_graph_socket() const
{
	return m_graph_in;
}

zsock_t * ZstGraphTransport::output_graph_socket() const
{
	return m_graph_out;
}

}
