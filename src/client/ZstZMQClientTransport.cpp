#include <czmq.h>
#include <sstream>

#include "../core/ZstEventDispatcher.hpp"
#include "../core/ZstZMQRefCounter.h"
#include "ZstZMQClientTransport.h"
#include "nlohmann/json.hpp"

ZstZMQClientTransport::ZstZMQClientTransport() : 
	m_server_sock(NULL)
{
}

ZstZMQClientTransport::~ZstZMQClientTransport()
{
}

void ZstZMQClientTransport::init()
{
	ZstTransportLayerBase::init();

	m_client_actor.init("client_actor");

	//Local dealer socket for receiving messages forwarded from other performers
	m_server_sock = zsock_new(ZMQ_DEALER);
	if (m_server_sock) {
		zsock_set_linger(m_server_sock, 0);
		m_client_actor.attach_pipe_listener(m_server_sock, s_handle_stage_router, this);
	}

	//Set up outgoing sockets
	zuuid_t * startup_uuid = zuuid_new();
	std::string identity = std::string(zuuid_str_canonical(startup_uuid));
	zuuid_destroy(&startup_uuid);

	zsock_set_identity(m_server_sock, identity.c_str());
	m_client_actor.start_loop();

	zst_zmq_inc_ref_count();
}

void ZstZMQClientTransport::destroy()
{
	ZstTransportLayerBase::destroy();
    ZstMessageSupervisor::destroy();

	m_client_actor.stop_loop();
	if(m_server_sock)
		zsock_destroy(&m_server_sock);
	m_client_actor.destroy();

	zst_zmq_dec_ref_count();
}

void ZstZMQClientTransport::connect(const std::string & address)
{
	m_stage_addr = std::string(address);

	std::stringstream addr;
    addr << "tcp://" << m_stage_addr; // << ":" << STAGE_ROUTER_PORT;
	m_server_addr = addr.str();

	zsock_connect(m_server_sock, "%s", m_server_addr.c_str());
}

void ZstZMQClientTransport::disconnect()
{
	zsock_disconnect(m_server_sock, "%s", m_server_addr.c_str());
}

void ZstZMQClientTransport::process_events()
{
	ZstTransportLayerBase::process_events();
	ZstMessageSupervisor::cleanup_response_messages();
}

void ZstZMQClientTransport::send_message_impl(ZstMessage * msg)
{
	ZstStageMessage* stage_msg = static_cast<ZstStageMessage*>(msg);
	zmsg_t * m = zmsg_new();

	//Insert empty frame at front of message to seperate between router sender hops and payloads
	zframe_t * spacer = zframe_new_empty();
	zmsg_prepend(m, &spacer);

	//Encode message as json
	zmsg_addstr(m, stage_msg->as_json_str().c_str());
	zmsg_send(&m, m_server_sock);

	//Errors
	int err = zmq_errno();
	if (err > 0) {
		ZstLog::net(LogLevel::error, "Message sending error: {}", zmq_strerror(err));
	}

	release_msg(stage_msg);
}

void ZstZMQClientTransport::sock_recv(zsock_t* socket, bool pop_first)
{
	if (!is_active())
		return;

	zmsg_t * recv_msg = zmsg_recv(socket);
	if (recv_msg) {
		if (pop_first) {
			zframe_t * empty = zmsg_pop(recv_msg);
			zframe_destroy(&empty);
		}
		char * msg_data_c = zmsg_popstr(recv_msg);

		ZstStageMessage * stage_msg = get_msg();
		stage_msg->unpack(json::parse(msg_data_c));
		zstr_free(&msg_data_c);
		receive_msg(stage_msg);
		zmsg_destroy(&recv_msg);
	}
}

int ZstZMQClientTransport::s_handle_stage_router(zloop_t * loop, zsock_t * sock, void * arg)
{
	ZstZMQClientTransport * transport = (ZstZMQClientTransport*)arg;
	transport->sock_recv(sock, true);
	return 0;
}

void ZstZMQClientTransport::receive_msg(ZstMessage * msg)
{
	ZstTransportLayer::receive_msg(msg, [msg, this](ZstEventStatus status) {
		this->release_msg(static_cast<ZstStageMessage*>(msg));
	});
}
