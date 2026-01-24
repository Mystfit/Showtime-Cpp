#include "../ZstZMQRefCounter.h"

#include <cstdlib>
#include <czmq.h>
#ifdef WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#endif
#include <showtime/ZstLogging.h>
#include "ZstGraphTransport.h"

namespace showtime {

ZstGraphTransport::ZstGraphTransport() :
	m_graph_in(NULL),
	m_graph_out(NULL),
	m_port(0)
{
}

ZstGraphTransport::~ZstGraphTransport()
{
    //m_graph_actor.stop_loop();
    destroy_graph_sockets();
}

void ZstGraphTransport::init()
{
	ZstTransportLayer::init();

	m_graph_actor.init("graph");
	init_graph_sockets();
	m_graph_actor.start_loop();
}

void ZstGraphTransport::destroy()
{
	m_graph_actor.stop_loop();
	destroy_graph_sockets();
	ZstTransportLayer::destroy();
}

const std::string & ZstGraphTransport::get_graph_in_address() const
{
	return m_graph_in_addr;
}

const std::string & ZstGraphTransport::get_graph_out_address() const
{
	return m_graph_out_addr;
}

//flatbuffers::DetachedBuffer ZstGraphTransport::create_msg(Content message_type, flatbuffers::Offset<void> message_content, flatbuffers::FlatBufferBuilder& buffer_builder)
//{
//	// Create the stage message
//	auto graph_msg = CreateGraphMessage(buffer_builder, );
//	FinishGraphMessageBuffer(buffer_builder, graph_msg);
//
//	/*auto verifier = flatbuffers::Verifier(buffer_builder.GetBufferPointer(), buffer_builder.GetSize());
//	if (!VerifyStageMessageBuffer(verifier))
//		throw;*/
//
//	return buffer_builder.Release();
//}
    
ZstMessageReceipt ZstGraphTransport::send_msg(flatbuffers::DetachedBuffer&& message_buffer, const ZstTransportArgs& args)
{
    send_message_impl(message_buffer, args);
    return ZstMessageReceipt{Signal_OK};
}

void ZstGraphTransport::send_message_impl(flatbuffers::DetachedBuffer& message_buffer, const ZstTransportArgs & args) const
{
	zframe_t * payload_frame = zframe_new(message_buffer.data(), message_buffer.size());
	zframe_set_group(payload_frame, PERFORMANCE_GROUP);
	zsock_t * sock = output_graph_socket();
	int result = (sock) ? zframe_send(&payload_frame, sock, 0) : -1;
	if (result < 0) {
		Log::net(Log::Level::warn, "Message failed to send with status {}", result);
	}
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

	if (in)
		zst_zmq_inc_ref_count("graphsocket_in");

	if (out)
		zst_zmq_inc_ref_count("graphsocket_out");
}

void ZstGraphTransport::set_graph_addresses(const std::string & in_addr, const std::string & out_addr)
{
	m_graph_out_addr = out_addr;
	m_graph_in_addr = in_addr;
	Log::net(Log::Level::debug, "Graph transport address in: {} out: {}", m_graph_in_addr, m_graph_out_addr);
}

int ZstGraphTransport::s_handle_graph_in(zloop_t * loop, zsock_t * sock, void * arg)
{
	ZstGraphTransport* transport = (ZstGraphTransport*)arg;
	transport->sock_recv(sock);
	return 0;
}

void ZstGraphTransport::sock_recv(zsock_t* socket)
{
	auto frame = zframe_recv(socket);
	//Unpack the frame into a message

	auto perf_msg = this->get_msg();
	auto owner = std::static_pointer_cast<ZstGraphTransport>(ZstTransportLayer::shared_from_this());
    perf_msg->init(GetGraphMessage(zframe_data(frame)), owner);
	
	//Publish message to other modules
    dispatch_receive_event(perf_msg, [perf_msg, frame](ZstEventStatus s) mutable {
		// Cleanup
		zframe_destroy(&frame);
	});
}

void ZstGraphTransport::destroy_graph_sockets()
{
    if (m_graph_in){
		//m_graph_actor.remove_pipe_listener(m_graph_in);
		zsock_destroy(&m_graph_in);
		m_graph_in = NULL;
		zst_zmq_dec_ref_count("graphsocket_in");
	}
    if (m_graph_out){
		//m_graph_actor.remove_pipe_listener(m_graph_out);
		zsock_destroy(&m_graph_out);
		m_graph_out = NULL;
		zst_zmq_dec_ref_count("graphsocket_out");
	}
}

zsock_t * ZstGraphTransport::input_graph_socket() const
{
	return m_graph_in;
}

zsock_t * ZstGraphTransport::output_graph_socket() const
{
	return m_graph_out;
}

void ZstGraphTransport::set_port(uint16_t port)
{
	m_port = port;
}

uint16_t ZstGraphTransport::get_port() {
	return m_port;
}

}
