#include <czmq.h>
#include <sstream>

#include "ZstServerSendTransport.h"
#include "nlohmann/json.hpp"

ZstServerSendTransport::ZstServerSendTransport() : ZstMessageSupervisor(std::make_shared<cf::time_watcher>(), STAGE_TIMEOUT)
{
}

ZstServerSendTransport::~ZstServerSendTransport()
{
}

void ZstServerSendTransport::init()
{
	ZstTransportLayerBase::init();

	m_client_actor.init("client_actor");

	//Local dealer socket for receiving messages forwarded from other performers
	m_stage_router = zsock_new(ZMQ_DEALER);
	if (m_stage_router) {
		zsock_set_linger(m_stage_router, 0);
		m_client_actor.attach_pipe_listener(m_stage_router, s_handle_stage_router, this);
	}

	//Set up outgoing sockets
	zuuid_t * startup_uuid = zuuid_new();
	std::string identity = std::string(zuuid_str_canonical(startup_uuid));
	zuuid_destroy(&startup_uuid);
	ZstLog::net(LogLevel::notification, "Setting socket identity to {}. Length {}", identity, identity.size());

	zsock_set_identity(m_stage_router, identity.c_str());
	m_client_actor.start_loop();
}

void ZstServerSendTransport::destroy()
{
	ZstTransportLayerBase::destroy();

	m_client_actor.stop_loop();
	if(m_stage_router)
		zsock_destroy(&m_stage_router);
	m_client_actor.destroy();
}

void ZstServerSendTransport::connect_to_stage(const std::string stage_address)
{
	m_stage_addr = std::string(stage_address);

	std::stringstream addr;
	addr << "tcp://" << m_stage_addr << ":" << STAGE_ROUTER_PORT;
	m_stage_router_addr = addr.str();

	zsock_connect(m_stage_router, "%s", m_stage_router_addr.c_str());
}

void ZstServerSendTransport::disconnect_from_stage()
{
	zsock_disconnect(m_stage_router, "%s", m_stage_router_addr.c_str());
}

void ZstServerSendTransport::process_events()
{
	ZstTransportLayerBase::process_events();
	ZstMessageSupervisor::cleanup_response_messages();
}

void ZstServerSendTransport::begin_send_message(ZstMessage * msg, const ZstTransportSendType & sendtype, const MessageReceivedAction & action)
{
	if (!msg) return;
	ZstStageMessage * stage_msg = static_cast<ZstStageMessage*>(msg);

	switch (sendtype) {
	case ZstTransportSendType::ASYNC_REPLY:
		send_async_message(stage_msg, action);
		break;
	case ZstTransportSendType::SYNC_REPLY:
		action(send_sync_message(stage_msg));
		break;
	case ZstTransportSendType::PUBLISH:
		send_message_impl(stage_msg);
		break;
	default:
		ZstTransportLayerBase::begin_send_message(stage_msg, sendtype, action);
		break;
	}
}

void ZstServerSendTransport::send_message_impl(ZstMessage * msg)
{
	zmsg_t * m = zmsg_new();

	//Insert empty frame at front of message to seperate between router sender hops and payloads
	zframe_t * spacer = zframe_new_empty();
	zmsg_prepend(m, &spacer);

	ZstStageMessage * stage_msg = static_cast<ZstStageMessage*>(msg);
	zmsg_addstr(m, stage_msg->as_json_str().c_str());
	zmsg_send(&m, m_stage_router);

	release_msg(stage_msg);
}

void ZstServerSendTransport::sock_recv(zsock_t* socket, bool pop_first)
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
		on_receive_msg(stage_msg);
		zmsg_destroy(&recv_msg);
	}
}

int ZstServerSendTransport::s_handle_stage_router(zloop_t * loop, zsock_t * sock, void * arg)
{
	ZstServerSendTransport * transport = (ZstServerSendTransport*)arg;
	transport->sock_recv(sock, true);
	return 0;
}

void ZstServerSendTransport::on_receive_msg(ZstMessage * msg)
{
	//Publish message to other modules
	msg_events()->defer([msg](ZstTransportAdaptor * adaptor) { 
		adaptor->on_receive_msg(msg); 
	}, [msg, this](ZstEventStatus status){ 
		ZstStageMessage * stage_msg = static_cast<ZstStageMessage*>(msg);

		//Process responses last to make sure our graph has been updated first
		process_response(stage_msg->id(), stage_msg->kind());
		this->release_msg(stage_msg);
	});
}

ZstMessageReceipt ZstServerSendTransport::send_sync_message(ZstStageMessage * msg)
{
	auto future = register_response(msg->id());
	ZstMessageReceipt msg_response{ ZstMsgKind::EMPTY, ZstTransportSendType::SYNC_REPLY };

	send_message_impl(msg);
	try {
		msg_response.status = future.get();
	}
	catch (const ZstTimeoutException & e) {
		ZstLog::net(LogLevel::error, "Server response timed out", e.what());
		msg_response.status = ZstMsgKind::ERR_STAGE_TIMEOUT;
	}

	enqueue_resolved_promise(msg->id());
	return msg_response;
}

void ZstServerSendTransport::send_async_message(ZstStageMessage * msg, const MessageReceivedAction & completed_action)
{
	auto future = register_response(msg->id());

	//Hold on to the message id so we can clean up the promise in case we time out
	ZstMsgID id = msg->id();

	future.then([this, id, completed_action](ZstMessageFuture f) {
		ZstMsgKind status(ZstMsgKind::EMPTY);
		ZstMessageReceipt msg_response{ status, ZstTransportSendType::ASYNC_REPLY };
		try {
			ZstMsgKind status = f.get();
			msg_response.status = status;
			completed_action(msg_response);
			return status;
		}
		catch (const ZstTimeoutException & e) {
			ZstLog::net(LogLevel::error, "Server async response timed out - {}", e.what());
			msg_response.status = ZstMsgKind::ERR_STAGE_TIMEOUT;
			completed_action(msg_response);
			enqueue_resolved_promise(id);
		}
		return status;
	});

	send_message_impl(msg);
}

