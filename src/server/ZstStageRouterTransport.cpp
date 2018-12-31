#include "ZstStageRouterTransport.h"
#include <czmq.h>
#include <sstream>

ZstStageRouterTransport::ZstStageRouterTransport()
{
}

ZstStageRouterTransport::~ZstStageRouterTransport()
{
}

void ZstStageRouterTransport::init(std::shared_ptr<ZstActor> reactor)
{
	ZstTransportLayerBase::init(reactor);

	std::stringstream addr;
	m_performer_router = zsock_new(ZMQ_ROUTER);
	zsock_set_linger(m_performer_router, 0);
	get_reactor()->attach_pipe_listener(m_performer_router, s_handle_router, this);

	addr << "tcp://*:" << STAGE_ROUTER_PORT;
	zsock_bind(m_performer_router, "%s", addr.str().c_str());
	if (!m_performer_router) {
		ZstLog::net(LogLevel::notification, "Could not bind stage router socket to {}", addr.str());
		return;
	}
	zsock_set_linger(m_performer_router, 0);
	zsock_set_router_mandatory(m_performer_router, 1);
	
	ZstLog::net(LogLevel::notification, "Stage router listening on address {}", addr.str());
}

void ZstStageRouterTransport::destroy()
{
	ZstTransportLayerBase::destroy();
	if(m_performer_router)
		zsock_destroy(&m_performer_router);
}


//------------------------
//Incoming socket handlers
//------------------------

int ZstStageRouterTransport::s_handle_router(zloop_t * loop, zsock_t * socket, void * arg)
{
	ZstStageRouterTransport * transport = (ZstStageRouterTransport*)arg;
	zmsg_t * recv_msg = zmsg_recv(socket);
	zmsg_print(recv_msg);

	if (recv_msg) {
		//Get identity of sender from first frame
		zframe_t * identity_frame = zmsg_pop(recv_msg);
		zframe_t * empty = zmsg_pop(recv_msg);

		//Unpack message
		char * payload_data = zmsg_popstr(recv_msg);
		ZstStageMessage * msg = transport->get_msg();
		msg->unpack(json::parse(payload_data));

		//Save sender as a local argument
		msg->set_arg<std::string, std::string>(get_msg_arg_name(ZstMsgArg::SENDER), std::string((char*)zframe_data(identity_frame), zframe_size(identity_frame)));

		zframe_destroy(&identity_frame);
		zframe_destroy(&empty);
		zstr_free(&payload_data);

		transport->on_receive_msg(msg);
	}

	return 0;
}

void ZstStageRouterTransport::send_message_impl(ZstMessage * msg)
{
	ZstStageMessage * stage_msg = static_cast<ZstStageMessage*>(msg);
	zmsg_t * m = zmsg_new();
	zmsg_addstr(m, stage_msg->get_arg<std::string>(ZstMsgArg::DESTINATION).c_str());
	zframe_t * empty = zframe_new_empty();
	zmsg_append(m, &empty);
	zmsg_addstr(m, stage_msg->as_json_str().c_str());
	zmsg_print(m);
	int rc = zmsg_send(&m, m_performer_router);
	int err = zmq_errno();
	if (err > 0) {
		if(err == EHOSTUNREACH)
			ZstLog::net(LogLevel::error, "Could not reach host");
		else
			ZstLog::net(LogLevel::error, "Message sending result code: {}", err);
	}
	release_msg(stage_msg);
}

void ZstStageRouterTransport::on_receive_msg(ZstMessage * msg)
{
	//Process response messages first
	this->ZstTransportLayerBase::on_receive_msg(msg);

	//Publish message to other modules
	msg_events()->defer([msg, this](ZstTransportAdaptor * adaptor) {
		adaptor->on_receive_msg(msg);
	}, [msg, this](ZstEventStatus status) {
		this->release_msg(static_cast<ZstStageMessage*>(msg));
	});
}
