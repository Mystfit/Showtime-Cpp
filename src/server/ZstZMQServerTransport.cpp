#include <czmq.h>
#include <sstream>

#include "../core/ZstZMQRefCounter.h"
#include "ZstZMQServerTransport.h"

ZstZMQServerTransport::ZstZMQServerTransport()
{
}

ZstZMQServerTransport::~ZstZMQServerTransport()
{
}

void ZstZMQServerTransport::init(int port)
{
	ZstTransportLayerBase::init();
	m_server_actor.init("stage_router");

	std::stringstream addr;
	m_clients_sock = zsock_new(ZMQ_ROUTER);
	zsock_set_linger(m_clients_sock, 0);
	zsock_set_router_mandatory(m_clients_sock, 1);
	m_server_actor.attach_pipe_listener(m_clients_sock, s_handle_router, this);

	addr << "tcp://*:" << port;
	zsock_bind(m_clients_sock, "%s", addr.str().c_str());
	if (!m_clients_sock) {
		ZstLog::net(LogLevel::notification, "Could not bind stage router socket to {}", addr.str());
		return;
	}
	
	ZstLog::net(LogLevel::notification, "Stage router listening on address {}", addr.str());
	m_server_actor.start_loop();

	//Increase the zmq context reference count
	zst_zmq_inc_ref_count();
}

void ZstZMQServerTransport::init()
{
	static_assert(true, "Removed: Use init(int port) instead");
}

void ZstZMQServerTransport::destroy()
{
	ZstTransportLayerBase::destroy();

	m_server_actor.stop_loop();
	if(m_clients_sock)
		zsock_destroy(&m_clients_sock);
	m_server_actor.destroy();

	//Decrease the zmq context reference count
	zst_zmq_dec_ref_count();
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
			msg->set_arg<std::string, std::string>(get_msg_arg_name(ZstMsgArg::SENDER), std::string((char*)zframe_data(identity_frame), zframe_size(identity_frame)));
			transport->on_receive_msg(msg);
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

void ZstZMQServerTransport::send_message_impl(ZstMessage * msg)
{
	ZstStageMessage * stage_msg = static_cast<ZstStageMessage*>(msg);
	zmsg_t * m = zmsg_new();

	//Add destination frame at beginning to route our message to the correct destination
	zmsg_addstr(m, stage_msg->get_arg<std::string>(ZstMsgArg::DESTINATION).c_str());

	//Spacer frame between destination and data
	zframe_t * empty = zframe_new_empty();
	zmsg_append(m, &empty);

	//Encode message as json
	zmsg_addstr(m, stage_msg->as_json_str().c_str());
	zmsg_send(&m, m_clients_sock);

	int err = zmq_errno();
	if (err > 0) {
		if(err == EHOSTUNREACH)
			ZstLog::net(LogLevel::error, "Could not reach host");
		else
			ZstLog::net(LogLevel::error, "Message sending error: {}", zmq_strerror(err));
	}
	release_msg(stage_msg);
}

void ZstZMQServerTransport::on_receive_msg(ZstMessage * msg)
{
	//Process response messages first
	this->ZstTransportLayerBase::on_receive_msg(msg);

	//Publish message to other modules
	msg_events()->defer([msg](ZstTransportAdaptor * adaptor) {
		adaptor->on_receive_msg(msg);
	}, [msg, this](ZstEventStatus status) {
		this->release_msg(static_cast<ZstStageMessage*>(msg));
	});
}
