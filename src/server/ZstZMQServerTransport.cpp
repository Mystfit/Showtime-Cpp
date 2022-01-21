#include <sstream>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include "../core/ZstZMQRefCounter.h"
#include "ZstZMQServerTransport.h"

#include <czmq.h>
#include <sstream>

namespace showtime {

ZstZMQServerTransport::ZstZMQServerTransport() :
	m_port(-1),
    m_clients_sock(NULL)
{
}

ZstZMQServerTransport::~ZstZMQServerTransport()
{
	destroy();
}

void ZstZMQServerTransport::init()
{
	ZstTransportLayer::init();

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

	//Allow this transport to send messages
	set_connected(true);

	//Increase the zmq context reference count
	zst_zmq_inc_ref_count("server_sock");
}

void ZstZMQServerTransport::destroy()
{
	m_server_actor.stop_loop();
	if (m_clients_sock) {
		//m_server_actor.remove_pipe_listener(m_clients_sock);
		zsock_destroy(&m_clients_sock);
		m_clients_sock = NULL;
		zst_zmq_dec_ref_count("server_sock");
	}

	m_port = -1;
	set_connected(false);
	ZstTransportLayer::destroy();
}

int ZstZMQServerTransport::bind(const std::string& address)
{
	std::stringstream addr;
	addr << "tcp://" << address;
	m_port = zsock_bind(m_clients_sock, "%s", addr.str().c_str());
	if (!m_clients_sock) {
		Log::net(Log::Level::notification, "Could not bind stage router socket to {}", addr.str());
		return m_port;
	}
	Log::net(Log::Level::notification, "Stage router listening on port {}", m_port);
	return m_port;
}

int ZstZMQServerTransport::port()
{
	return m_port;
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
		auto identity_frame = zmsg_pop(recv_msg);
		auto empty = zmsg_pop(recv_msg);
		auto id_frame = zmsg_pop(recv_msg);
		auto payload_data = zmsg_pop(recv_msg);

		// Copy msg id to UUID
		ZstMsgID msg_id;
		memcpy(&msg_id, zframe_data(id_frame), zframe_size(id_frame));

		// Get client UUID
		auto client_uuid = uuid(boost::lexical_cast<uuid>((char*)zframe_data(identity_frame), zframe_size(identity_frame)));

		//Unpack message
		if (payload_data) {
			auto verifier = flatbuffers::Verifier(zframe_data(payload_data), zframe_size(payload_data));
			if (VerifyStageMessageBuffer(verifier)) {
				auto msg = get_msg();
				auto owner = std::dynamic_pointer_cast<ZstStageTransport>(ZstTransportLayer::shared_from_this());
				msg->init(
					GetStageMessage(zframe_data(payload_data)),
					client_uuid,
					msg_id,
					owner
				);

				// Send message to submodules
				dispatch_receive_event(msg, [msg, identity_frame, payload_data](ZstEventStatus s) mutable {
					// Frame cleanup
					zframe_destroy(&payload_data);
					zframe_destroy(&identity_frame);
				});
			}
			else {
				Log::server(Log::Level::warn, "Received malformed message. Alerting client!");
				signal_client_direct(Signal_ERR_MSG_MALFORMED, msg_id, client_uuid);
			}
		}
		else {
			Log::server(Log::Level::warn, "Received message with no payload data. Can't unpack.");
		}

		//Cleanup resources
		zframe_destroy(&empty);
		zmsg_destroy(&recv_msg);
	}
}

void ZstZMQServerTransport::send_message_impl(const uint8_t* msg_buffer, size_t msg_buffer_size, const ZstTransportArgs& args) const
{
	auto verifier = flatbuffers::Verifier(msg_buffer, msg_buffer_size);
	if (!VerifyStageMessageBuffer(verifier))
		throw;

	zmsg_t* m = zmsg_new();

	//Add destination frame at beginning to route our message to the correct destination
	zmsg_addstr(m, to_string(args.target_endpoint_UUID).c_str());

	//Spacer frame between destination and data
	zframe_t* empty = zframe_new_empty();
	zmsg_append(m, &empty);

	// Add message ID
	zmsg_addmem(m, args.msg_ID.data, 16);

	//Encode message from flatbuffers to bytes
	zmsg_addmem(m, msg_buffer, msg_buffer_size);

	int result = 0;
	{
		std::lock_guard<std::mutex> lock(m_transport_mtx);
		//int result = m_server_actor.send_to_socket(m_clients_sock, m);
		result = zmsg_send(&m, m_clients_sock);
	}

	//Errors
	if (result != 0) {
		int err = zmq_errno();
		if (err > 0) {
			Log::net(Log::Level::error, "Server message sending error: {}", zmq_strerror(err));
		}
	}
}

void ZstZMQServerTransport::signal_client_direct(Signal signal, ZstMsgID msg_id, const boost::uuids::uuid& client)
{
	flatbuffers::FlatBufferBuilder builder;
	auto signal_offset = CreateSignalMessage(builder, signal);
	auto msg_offset = CreateStageMessage(builder, Content_SignalMessage, signal_offset.Union());
	FinishStageMessageBuffer(builder, msg_offset);
	
	ZstTransportArgs args;
	args.target_endpoint_UUID = client;
	args.msg_ID = msg_id;
	send_message_impl(builder.GetBufferPointer(), builder.GetSize(), args);
}

}
