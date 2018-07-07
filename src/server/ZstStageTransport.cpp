#include "ZstStageTransport.h"
#include "../core/ZstTransportDispatcher.h"

ZstStageTransport::ZstStageTransport()
{
	m_message_pool = new ZstMessagePool<ZstStageMessage>();
	m_message_pool->populate(STAGE_MESSAGE_POOL_BLOCK);
}

ZstStageTransport::~ZstStageTransport()
{
	delete m_message_pool;
}

void ZstStageTransport::init()
{
	ZstActor::init();

	std::stringstream addr;
	m_performer_router = zsock_new(ZMQ_ROUTER);
	zsock_set_linger(m_performer_router, 0);
	attach_pipe_listener(m_performer_router, s_handle_router, this);

	addr << "tcp://*:" << STAGE_ROUTER_PORT;
	zsock_bind(m_performer_router, "%s", addr.str().c_str());
	if (!m_performer_router) {
		ZstLog::net(LogLevel::notification, "Could not bind stage router socket to {}", addr.str());
		return;
	}
	addr.str("");

	addr << "@tcp://*:" << STAGE_PUB_PORT;
	m_graph_update_pub = zsock_new_pub(addr.str().c_str());
	if (!m_graph_update_pub) {
		ZstLog::net(LogLevel::notification, "Could not bind stage graph publisher socket to {}", addr.str());
		return;
	}
	zsock_set_linger(m_graph_update_pub, 0);
	
	start_loop();
}

void ZstStageTransport::destroy()
{
	ZstActor::destroy();
	zsock_destroy(&m_performer_router);
	zsock_destroy(&m_graph_update_pub);
	zsys_shutdown();
}


//------------------------
//Incoming socket handlers
//------------------------

int ZstStageTransport::s_handle_router(zloop_t * loop, zsock_t * socket, void * arg)
{
	ZstStageTransport * transport = (ZstStageTransport*)arg;
	zframe_t * identity_frame = NULL;
	std::string sender_identity;

	//Receive waiting message
	zmsg_t * recv_msg = zmsg_recv(socket);
	ZstStageMessage * msg = NULL;

	if (recv_msg) {
		//Get identity of sender from first frame
		zframe_t * identity_frame = zmsg_pop(recv_msg);
		zframe_t * empty = zmsg_pop(recv_msg);

		msg = transport->msg_pool()->get_msg();
		msg->unpack(recv_msg);

		//HACK: Store the sender UUID in the URI sender
		msg->set_sender(ZstURI((char*)zframe_data(identity_frame), zframe_size(identity_frame)));

		zframe_destroy(&identity_frame);
		zframe_destroy(&empty);

		transport->msg_dispatch()->receive_stage_msg(msg);
	}
}



//--------------------
//Client communication
//--------------------

void ZstStageTransport::send_to_client(ZstStageMessage * msg, ZstPerformer * destination)
{
	zmsg_t * msg_handle = msg->handle();
	zframe_t * identity = zframe_from(get_socket_ID(destination).c_str());
	zframe_t * empty = zframe_new_empty();
	zmsg_prepend(msg_handle, &empty);
	zmsg_prepend(msg_handle, &identity);
	zmsg_send(&msg_handle, m_performer_router);
	msg_pool()->release(msg);
}

void ZstStageTransport::publish_stage_update(ZstStageMessage * msg)
{
	zmsg_t * msg_handle = msg->handle();
	zmsg_send(&msg_handle, m_graph_update_pub);
	msg_pool()->release(msg);
}

ZstMessagePool<ZstStageMessage>* ZstStageTransport::msg_pool()
{
	return m_message_pool;
}
