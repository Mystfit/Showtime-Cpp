#include <czmq.h>
#include <sstream>
#include <fmt/format.h>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include "../core/ZstZMQRefCounter.h"
#include "ZstZMQServerTransport.h"

namespace showtime {

ZstZMQServerTransport::ZstZMQServerTransport() :
	m_clients_sock(NULL)
{
}

ZstZMQServerTransport::~ZstZMQServerTransport()
{
	destroy();
}

void ZstZMQServerTransport::init()
{
	ZstTransportLayerBase::init();

	//Socket
	m_clients_sock = zsock_new(ZMQ_ROUTER);
	zsock_set_sndhwm(m_clients_sock, 1000);
	zsock_set_linger(m_clients_sock, 0);
	zsock_set_sndtimeo(m_clients_sock, 10);
	zsock_set_router_mandatory(m_clients_sock, 1);

	//Actor
	m_server_actor.init("stage_router");
	m_server_actor.attach_pipe_listener(m_clients_sock, s_handle_router, this);
	m_server_actor.start_loop();

	//Increase the zmq context reference count
	zst_zmq_inc_ref_count();
}

void ZstZMQServerTransport::destroy()
{
	m_server_actor.stop_loop();
	if (m_clients_sock) {
		//m_server_actor.remove_pipe_listener(m_clients_sock);
		zsock_destroy(&m_clients_sock);
		m_clients_sock = NULL;
		zst_zmq_dec_ref_count();
	}
	ZstTransportLayerBase::destroy();
}

void ZstZMQServerTransport::bind(const std::string& address)
{
	auto addr = fmt::format("tcp://{}", address);
	zsock_bind(m_clients_sock, "%s", addr.c_str());
	if (!m_clients_sock) {
		ZstLog::net(LogLevel::notification, "Could not bind stage router socket to {}", addr);
		return;
	}
	ZstLog::net(LogLevel::notification, "Stage router listening at address {}", addr);
}

//------------------------
//Incoming socket handlers
//------------------------

int ZstZMQServerTransport::s_handle_router(zloop_t* loop, zsock_t* socket, void* arg)
{
	ZstZMQServerTransport* transport = (ZstZMQServerTransport*)arg;
	transport->sock_recv(socket);
	return 0;
}

void ZstZMQServerTransport::sock_recv(zsock_t* socket)
{
	zmsg_t* recv_msg = zmsg_recv(socket);
	if (recv_msg) {
		//Get identity of sender from first frame
		zframe_t* identity_frame = zmsg_pop(recv_msg);
		zframe_t* empty = zmsg_pop(recv_msg);

		//Unpack message
		auto payload_data = zmsg_pop(recv_msg);
		if (payload_data) {
			auto msg = get_msg();
			msg->init(GetStageMessage(zframe_data(payload_data)));

			//Save sender as a local argument
			msg->set_endpoint_UUID(uuid(boost::lexical_cast<uuid>((char*)zframe_data(identity_frame), zframe_size(identity_frame))));
		
			// Send message to submodules
			dispatch_receive_event(msg, [this, msg, identity_frame, payload_data](ZstEventStatus s) mutable {
				// Frame cleanup
				zframe_destroy(&payload_data);
				zframe_destroy(&identity_frame);
			});
		}
		else {
			ZstLog::net(LogLevel::warn, "Received message with no payload data. Can't unpack.");
		}

		//Cleanup resources
		zframe_destroy(&empty);
		zmsg_destroy(&recv_msg);
	}
}

void ZstZMQServerTransport::send_message_impl(const uint8_t* msg_buffer, size_t msg_buffer_size, const ZstTransportArgs& args) const
{
	zmsg_t* m = zmsg_new();

	//Add destination frame at beginning to route our message to the correct destination
	zmsg_addstr(m, to_string(args.target_endpoint_UUID).c_str());

	//Spacer frame between destination and data
	zframe_t* empty = zframe_new_empty();
	zmsg_append(m, &empty);

	//Encode message from flatbuffers to bytes
	zmsg_addmem(m, msg_buffer, msg_buffer_size);

	std::lock_guard<std::mutex> lock(m_transport_mtx);
	//zmsg_send(&m, m_clients_sock);
	int result = m_server_actor.send_to_socket(m_clients_sock, m);

	//Errors
	if (result != 0) {
		int err = zmq_errno();
		if (err > 0) {
			ZstLog::net(LogLevel::error, "Server message sending error: {}", zmq_strerror(err));
		}
	}
}

}