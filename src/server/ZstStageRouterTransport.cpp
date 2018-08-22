#include <boost/lexical_cast.hpp>
#include "ZstStageRouterTransport.h"

ZstStageRouterTransport::ZstStageRouterTransport()
{
}

ZstStageRouterTransport::~ZstStageRouterTransport()
{
}

void ZstStageRouterTransport::init()
{
	ZstTransportLayerBase::init();
	m_router_actor.init("stage_router");

	std::stringstream addr;
	m_performer_router = zsock_new(ZMQ_ROUTER);
	zsock_set_linger(m_performer_router, 0);
	m_router_actor.attach_pipe_listener(m_performer_router, s_handle_router, this);

	addr << "tcp://*:" << STAGE_ROUTER_PORT;
	zsock_bind(m_performer_router, "%s", addr.str().c_str());
	if (!m_performer_router) {
		ZstLog::net(LogLevel::notification, "Could not bind stage router socket to {}", addr.str());
		return;
	}
	zsock_set_linger(m_performer_router, 0);

	m_router_actor.start_loop();
}

void ZstStageRouterTransport::destroy()
{
	ZstTransportLayerBase::destroy();

	m_router_actor.stop_loop();

	if(m_performer_router)
		zsock_destroy(&m_performer_router);

	m_router_actor.destroy();
}


//------------------------
//Incoming socket handlers
//------------------------

int ZstStageRouterTransport::s_handle_router(zloop_t * loop, zsock_t * socket, void * arg)
{
	ZstStageRouterTransport * transport = (ZstStageRouterTransport*)arg;
	zmsg_t * recv_msg = zmsg_recv(socket);
	if (recv_msg) {
		//Get identity of sender from first frame
		zframe_t * identity_frame = zmsg_pop(recv_msg);
		zframe_t * empty = zmsg_pop(recv_msg);

		//Unpack message
		ZstStageMessage * msg = transport->get_msg();
		msg->unpack(recv_msg);

		//Save sender as a local argument
		msg->set_local_arg(ZstMsgArg::SENDER_IDENTITY, std::string((char*)zframe_data(identity_frame), zframe_size(identity_frame)));

		zframe_destroy(&identity_frame);
		zframe_destroy(&empty);

		transport->on_receive_msg(msg);
	}

	return 0;
}

void ZstStageRouterTransport::send_message_impl(ZstMessage * msg)
{
	try {
		msg->set_id(boost::lexical_cast<ZstMsgID>(msg->get_arg(ZstMsgArg::MSG_ID)));
	}
	catch (std::out_of_range) {
	}

	std::string identity_s = msg->get_arg(ZstMsgArg::SENDER_IDENTITY);
	zmsg_t * msg_handle = msg->handle();
	zframe_t * identity = zframe_from(identity_s.c_str()); //get_socket_ID(destination).c_str()
	zframe_t * empty = zframe_new_empty();
	zmsg_prepend(msg_handle, &empty);
	zmsg_prepend(msg_handle, &identity);
	zmsg_send(&msg_handle, m_performer_router);
	release_msg(static_cast<ZstStageMessage*>(msg));
}

void ZstStageRouterTransport::on_receive_msg(ZstMessage * msg)
{
	ZstTransportLayerBase::on_receive_msg(msg);

	//Publish message to other modules
	msg_events()->invoke([msg](ZstTransportAdaptor * adaptor) {
		adaptor->on_receive_msg(msg);
	});
	release_msg(static_cast<ZstStageMessage*>(msg));
}
