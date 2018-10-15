#include <boost/lexical_cast.hpp>
#include "ZstTransportLayerBase.hpp"
#include "adaptors/ZstTransportAdaptor.hpp"


ZstTransportLayerBase::ZstTransportLayerBase() :
	m_dispatch_events(NULL),
	m_is_active(false)
{
	m_dispatch_events = new ZstEventDispatcher<ZstTransportAdaptor*>("msgdispatch stage events");
}

ZstTransportLayerBase::~ZstTransportLayerBase()
{
	delete m_dispatch_events;
}

void ZstTransportLayerBase::destroy()
{
	delete m_timeout_watcher;
	m_dispatch_events->flush();
	m_dispatch_events->remove_all_adaptors();
	m_is_active = false;
}

void ZstTransportLayerBase::init()
{
	m_timeout_watcher = new cf::time_watcher();
	m_is_active = true;
}

void ZstTransportLayerBase::send_sock_msg(zsock_t * sock, ZstMessage * msg)
{
	assert(msg);
	zmsg_t * handle = msg->handle();
	zmsg_send(&handle, sock);
}

zmsg_t * ZstTransportLayerBase::sock_recv(zsock_t* socket, bool pop_first)
{
	if (!is_active())
		return NULL;

	zmsg_t * recv_msg = zmsg_recv(socket);
	if (recv_msg) {
		if (pop_first) {
			zframe_t * empty = zmsg_pop(recv_msg);
			zframe_destroy(&empty);
		}
	}

	return recv_msg;
}

ZstEventDispatcher<ZstTransportAdaptor*>* ZstTransportLayerBase::msg_events()
{
	return m_dispatch_events;
}

bool ZstTransportLayerBase::is_active()
{
	return m_is_active;
}

void ZstTransportLayerBase::process_events()
{
	m_dispatch_events->process_events();

	ZstMsgID id;
	while (m_dead_promises.try_dequeue(id)) {
		cleanup_response_message(id);
	}
}

void ZstTransportLayerBase::begin_send_message(ZstMessage * msg)
{
	if (!msg) return;

	copy_id_arg(msg);
	send_message_impl(msg);
}

void ZstTransportLayerBase::begin_send_message(ZstMessage * msg, const ZstTransportSendType & sendtype, const MessageReceivedAction & action)
{
	if (!msg) return;

	copy_id_arg(msg);

	switch (sendtype) {
	case ZstTransportSendType::ASYNC_REPLY:
		send_async_message(msg, action);
		break;
	case ZstTransportSendType::SYNC_REPLY:
		action(send_sync_message(msg));
		break;
	case ZstTransportSendType::PUBLISH:
		send_message_impl(msg);
		break;
	default:
		break;
	}
}

void ZstTransportLayerBase::on_receive_msg(ZstMessage * msg)
{
	process_responses(msg);
}

ZstMessageReceipt ZstTransportLayerBase::send_sync_message(ZstMessage * msg)
{
	MessageFuture future = register_response_message(msg);
	ZstMessageReceipt msg_response{ ZstMsgKind::EMPTY, ZstTransportSendType::SYNC_REPLY };
	
    send_message_impl(msg);
	try {
		msg_response.status = future.get();
	}
	catch (const ZstTimeoutException & e) {
		ZstLog::net(LogLevel::error, "Server response timed out", e.what());
		msg_response.status = ZstMsgKind::ERR_STAGE_TIMEOUT;
		m_dead_promises.enqueue(msg->id());
	}
	return msg_response;
}

void ZstTransportLayerBase::send_async_message(ZstMessage * msg, const MessageReceivedAction & completed_action)
{
	MessageFuture future = register_response_message(msg);

	//Hold on to the message id so we can clean up the promise in case we time out
	ZstMsgID id = msg->id();

	future.then([this, id, completed_action](MessageFuture f) {
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
			m_dead_promises.enqueue(id);
		}
		return status;
	});

	send_message_impl(msg);
}

MessageFuture ZstTransportLayerBase::register_response_message(ZstMessage * msg)
{
	ZstMsgID id = msg->id();
	m_response_promises.emplace(id, MessagePromise());
	MessageFuture future = m_response_promises[id].get_future();
	future = future.timeout(std::chrono::milliseconds(STAGE_TIMEOUT), ZstTimeoutException("Connect timeout"), *m_timeout_watcher);
	return future;
}

void ZstTransportLayerBase::cleanup_response_message(ZstMsgID id)
{
	//Clear completed promise when finished
	try {
		m_response_promises.erase(id);
	}
	catch (std::out_of_range e) {
		ZstLog::net(LogLevel::debug, "Promise already removed. {}", e.what());
	}
}

void ZstTransportLayerBase::process_responses(ZstMessage * msg)
{
	int status = 0;
	try {
		ZstMsgID id = msg->id();
		m_response_promises.at(id).set_value(msg->kind());
		cleanup_response_message(id);
		status = 1;
	}
	catch (std::out_of_range e) {
		status = -1;
	}
}

void ZstTransportLayerBase::copy_id_arg(ZstMessage * msg)
{
	//If the message has a MSG_ID argument, copy it to the id portion of the message
	if(msg->has_arg(ZstMsgArg::MSG_ID))
		msg->set_id(boost::lexical_cast<ZstMsgID>(msg->get_arg(ZstMsgArg::MSG_ID)));
}
