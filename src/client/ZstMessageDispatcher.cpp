#include "ZstMessageDispatcher.h"

ZstMessageDispatcher::ZstMessageDispatcher(ZstClient * client, ZstTransportLayer * transport) :
	ZstClientModule(client),
	m_transport(transport)
{	
}

ZstMessageDispatcher::~ZstMessageDispatcher()
{
}

ZstMessageReceipt ZstMessageDispatcher::send_to_stage(ZstMessage * msg, MessageBoundAction action, bool async)
{
	if (!msg) return ZstMessageReceipt{ ZstMsgKind::EMPTY, async };

	if (async) {
		return send_async_stage_message(msg, action);
	}

	//A sync message can run its callback action as soon as it finishes
	ZstMessageReceipt msg_response = send_sync_stage_message(msg);
	action(msg_response);
	return msg_response;
}

ZstMessageReceipt ZstMessageDispatcher::send_sync_stage_message(ZstMessage * msg)
{
	MessageFuture future = register_response_message(msg);
	ZstMessageReceipt msg_response{ZstMsgKind::EMPTY, false};
	m_transport->send_to_stage(msg);
	try {
		msg_response.status = future.get();
		complete(msg_response);
	}
	catch (const ZstTimeoutException & e) {
		ZstLog::net(LogLevel::error, "Server response timed out", e.what());
		msg_response.status = ZstMsgKind::ERR_STAGE_TIMEOUT;
		failed(msg_response);
	}
	return msg_response;
}

ZstMessageReceipt ZstMessageDispatcher::send_async_stage_message(ZstMessage * msg, MessageBoundAction completed_action)
{
	MessageFuture future = register_response_message(msg);
	future.then([this, completed_action](MessageFuture f) {
		ZstMsgKind status;
		ZstMessageReceipt msg_response{ZstMsgKind::EMPTY, true};
		try {
			ZstMsgKind status = f.get();
			msg_response.status = status;
			completed_action(msg_response);
			complete(msg_response);
			return status;
		}
		catch (const ZstTimeoutException & e) {
			ZstLog::net(LogLevel::error, "Server async response timed out - {}", e.what());
			msg_response.status = ZstMsgKind::ERR_STAGE_TIMEOUT;
			failed(msg_response);
		}
		return status;
	});
	m_transport->send_to_stage(msg);
}

void ZstMessageDispatcher::complete(ZstMessageReceipt response)
{
	//If we didn't receive a OK signal, something went wrong
	if (response.status != ZstMsgKind::OK) {
        ZstLog::net(LogLevel::error, "Server response failed with status: {}", response.status);
        return;
	}
}

void ZstMessageDispatcher::failed(ZstMessageReceipt response)
{
}

ZstMessage * ZstMessageDispatcher::init_entity_message(const ZstEntityBase * entity)
{
	ZstMessage * msg = m_transport->get_msg();
	msg->reset();
	msg->append_id_frame();
	msg->append_entity_kind_frame(entity);
	msg->append_payload_frame(*entity);
	return msg;
}

ZstMessage * ZstMessageDispatcher::init_message(ZstMsgKind kind)
{
	ZstMessage * msg = m_transport->get_msg();
	msg->reset();
	msg->append_id_frame();
	msg->append_kind_frame(kind);
	return msg;
}

ZstMessage * ZstMessageDispatcher::init_serialisable_message(ZstMsgKind kind, const ZstSerialisable & serialisable)
{
	ZstMessage * msg = m_transport->get_msg();
	msg->reset();
	msg->append_id_frame();
	msg->append_kind_frame(kind);
	msg->append_payload_frame(serialisable);
	return msg;
}


MessageFuture ZstMessageDispatcher::register_response_message(ZstMessage * msg)
{
	std::string id = std::string(msg->id());
	m_promise_messages[id] = MessagePromise();
	MessageFuture future = m_promise_messages[id].get_future();
	future = future.timeout(std::chrono::milliseconds(STAGE_TIMEOUT), ZstTimeoutException("Connect timeout"), m_timeout_watcher);
	return future;
}

int ZstMessageDispatcher::process_response_message(ZstMessage * msg)
{
	int status = 0;
	try {
		std::string id = std::string(msg->id());
		m_promise_messages.at(id).set_value(msg->kind());

		//Clear completed promise when finished
		m_promise_messages.erase(msg->id());
		status = 1;
	}
	catch (std::out_of_range e) {
		status = -1;
	}

	return status;
}
