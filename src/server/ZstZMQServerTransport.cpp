#include <czmq.h>
#include <sstream>
#include <fmt/format.h>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include "../core/ZstZMQRefCounter.h"
#include "ZstZMQServerTransport.h"

ZstZMQServerTransport::ZstZMQServerTransport() :
	m_clients_sock(NULL)
{
}

ZstZMQServerTransport::~ZstZMQServerTransport()
{
    //m_server_actor.stop_loop();
    if(m_clients_sock){
		m_server_actor.remove_pipe_listener(m_clients_sock);
        zsock_destroy(&m_clients_sock);
        zst_zmq_dec_ref_count();
    }
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
	if (m_clients_sock) {
		//m_server_actor.remove_pipe_listener(m_clients_sock);
		zsock_destroy(&m_clients_sock);
		m_clients_sock = NULL;
		zst_zmq_dec_ref_count();
	}
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

int ZstZMQServerTransport::s_handle_router(zloop_t * loop, zsock_t * socket, void * arg)
{
	ZstZMQServerTransport * transport = (ZstZMQServerTransport*)arg;
	zmsg_t * recv_msg = zmsg_recv(socket);
	if (recv_msg) {
		//Get identity of sender from first frame
		zframe_t * identity_frame = zmsg_pop(recv_msg);
		zframe_t * empty = zmsg_pop(recv_msg);

		//Unpack message
		char * payload_data = zmsg_popstr(recv_msg);
		if (payload_data) {
			ZstStageMessage * msg = transport->get_msg();
			msg->unpack(json::parse(payload_data));

			//Save sender as a local argument
			msg->set_endpoint_UUID(uuid(boost::lexical_cast<uuid>((char*)zframe_data(identity_frame), zframe_size(identity_frame))));
			ZstLog::net(LogLevel::debug, "Server receiving message. Msg id {}", msg->as_json_str());
			transport->receive_msg(msg);
		}
		else {
			ZstLog::net(LogLevel::warn, "Received message with no payload data. Can't unpack.");
		}

		//Cleanup resources
		zframe_destroy(&identity_frame);
		zframe_destroy(&empty);
		if(payload_data)
			zstr_free(&payload_data);
	}

	return 0;
}

void ZstZMQServerTransport::send_message_impl(ZstMessage * msg, const ZstTransportArgs& args)
{
	ZstStageMessage * stage_msg = static_cast<ZstStageMessage*>(msg);
	ZstLog::net(LogLevel::debug, "Server sending message. Msg id {}", stage_msg->as_json_str());

	zmsg_t * m = zmsg_new();

	//Add destination frame at beginning to route our message to the correct destination
	zmsg_addstr(m, to_string(args.target_endpoint_UUID).c_str());

	//Spacer frame between destination and data
	zframe_t * empty = zframe_new_empty();
	zmsg_append(m, &empty);

	//Encode message as json
	zmsg_addstr(m, stage_msg->as_json_str().c_str());

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
	release_msg(stage_msg);
}

void ZstZMQServerTransport::receive_msg(ZstMessage * msg)
{
	ZstTransportLayer::receive_msg(msg, [msg, this](ZstEventStatus status) {
		this->release_msg(static_cast<ZstStageMessage*>(msg));
	});
}
