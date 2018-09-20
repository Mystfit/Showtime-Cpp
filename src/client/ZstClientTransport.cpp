#include <boost/lexical_cast.hpp>
#include "ZstClientTransport.h"
#include "../core/ZstStageMessage.h"

ZstClientTransport::ZstClientTransport()
{
}

ZstClientTransport::~ZstClientTransport()
{
}

void ZstClientTransport::init()
{
	ZstTransportLayerBase::init();

	m_client_actor.init("client_actor");
	
	m_startup_uuid = zuuid_new();

	//Local dealer socket for receiving messages forwarded from other performers
	m_stage_router = zsock_new(ZMQ_DEALER);
	if (m_stage_router) {
		zsock_set_linger(m_stage_router, 0);
		m_client_actor.attach_pipe_listener(m_stage_router, s_handle_stage_router, this);
	}

	m_stage_updates = zsock_new(ZMQ_SUB);
	if (m_stage_updates) {
		zsock_set_linger(m_stage_updates, 0);
		m_client_actor.attach_pipe_listener(m_stage_updates, s_handle_stage_update_in, this);
	}

	//Set up outgoing sockets
	std::string identity = std::string(zuuid_str_canonical(m_startup_uuid));
	ZstLog::net(LogLevel::notification, "Setting socket identity to {}. Length {}", identity, identity.size());

	zsock_set_identity(m_stage_router, identity.c_str());
	m_client_actor.start_loop();
}

void ZstClientTransport::destroy()
{
	ZstTransportLayerBase::destroy();
	m_client_actor.stop_loop();
	if(m_stage_updates)
		zsock_destroy(&m_stage_updates);
	if(m_stage_router)
		zsock_destroy(&m_stage_router);
	m_client_actor.destroy();
}

void ZstClientTransport::connect_to_stage(std::string stage_address)
{
	m_stage_addr = std::string(stage_address);

	std::stringstream addr;
	addr << "tcp://" << m_stage_addr << ":" << STAGE_ROUTER_PORT;
	m_stage_router_addr = addr.str();

	zsock_connect(m_stage_router, "%s", m_stage_router_addr.c_str());
	addr.str("");

	//Stage subscriber socket for update messages
	addr << "tcp://" << m_stage_addr << ":" << STAGE_PUB_PORT;
	m_stage_updates_addr = addr.str();

	ZstLog::net(LogLevel::notification, "Connecting to stage publisher {}", m_stage_updates_addr);
	zsock_connect(m_stage_updates, "%s", m_stage_updates_addr.c_str());
	zsock_set_subscribe(m_stage_updates, "");
	addr.str("");
}

void ZstClientTransport::disconnect_from_stage()
{
	zsock_disconnect(m_stage_updates, "%s", m_stage_updates_addr.c_str());
	zsock_disconnect(m_stage_router, "%s", m_stage_router_addr.c_str());
}

void ZstClientTransport::send_message_impl(ZstMessage * msg)
{
	//Insert empty frame at front of message to seperate between router sender hops and payloads
	zframe_t * spacer = zframe_new_empty();
	zmsg_prepend(msg->handle(), &spacer);
	ZstTransportLayer<ZstStageMessage>::send_sock_msg(m_stage_router, static_cast<ZstStageMessage*>(msg));
}

int ZstClientTransport::s_handle_stage_update_in(zloop_t * loop, zsock_t * sock, void * arg)
{
	ZstClientTransport * transport = (ZstClientTransport*)arg;
	ZstStageMessage * stage_msg = transport->get_msg();
	zmsg_t * sock_msg = transport->sock_recv(sock, false);
	if (sock_msg) {
		stage_msg->unpack(sock_msg);
		transport->on_receive_msg(stage_msg);
		zmsg_destroy(&sock_msg);
	}
	return 0;
}

int ZstClientTransport::s_handle_stage_router(zloop_t * loop, zsock_t * sock, void * arg)
{
	ZstClientTransport * transport = (ZstClientTransport*)arg;
	ZstStageMessage * stage_msg = transport->get_msg();
	zmsg_t * sock_msg = transport->sock_recv(sock, true);
	if (sock_msg) {
		stage_msg->unpack(sock_msg);
		transport->on_receive_msg(stage_msg);
		zmsg_destroy(&sock_msg);
	}
	return 0;
}

void ZstClientTransport::on_receive_msg(ZstMessage * msg)
{
	ZstTransportLayerBase::on_receive_msg(msg);

	//Publish message to other modules
	msg_events()->invoke([msg](ZstTransportAdaptor * adaptor) {
		adaptor->on_receive_msg(msg);
	});
	release_msg(static_cast<ZstStageMessage*>(msg));
}
