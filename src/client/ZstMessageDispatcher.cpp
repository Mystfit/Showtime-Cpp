#include "ZstMessageDispatcher.h"

ZstMessageDispatcher::ZstMessageDispatcher() :
	ZstClientModule(),
	m_transport(NULL)
{	
}

ZstMessageDispatcher::~ZstMessageDispatcher()
{
}

void ZstMessageDispatcher::set_transport(ZstTransportLayer * transport)
{
	m_transport = transport;
}

void ZstMessageDispatcher::process_events()
{
	ZstEventDispatcher<ZstStageDispatchAdaptor*>::process_events();
	ZstEventDispatcher<ZstPerformanceDispatchAdaptor*>::process_events();
}

void ZstMessageDispatcher::send_to_stage(ZstMessage * msg, bool async, MessageReceivedAction action)
{
	if (!msg) return;

	if (async) {
		send_async_stage_message(msg, action);
		return;
	}

	//A sync message can run its callback action as soon as it finishes
	action(send_sync_stage_message(msg));
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

void ZstMessageDispatcher::send_async_stage_message(ZstMessage * msg, MessageReceivedAction completed_action)
{
	MessageFuture future = register_response_message(msg);
	future.then([this, completed_action](MessageFuture f) {
		ZstMsgKind status(ZstMsgKind::EMPTY);
		ZstMessageReceipt msg_response{status, true};
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

void ZstMessageDispatcher::send_to_performance(ZstPlug * plug)
{
	ZstMessage * msg = transport()->get_msg()->init_performance_message(plug);
	m_transport->send_to_performance(msg);
}

void ZstMessageDispatcher::receive_from_performance(ZstMessage * msg)
{
	//Forward stage message to all adaptors
	run_event([msg](ZstPerformanceDispatchAdaptor * adaptor) {
		adaptor->on_receive_from_performance(msg);
	});
}

void ZstMessageDispatcher::receive_from_stage(size_t payload_index, ZstMessage * msg)
{
	//Forward stage message to all adaptors
	run_event([payload_index, msg](ZstStageDispatchAdaptor * adaptor) {
		adaptor->on_receive_from_stage(payload_index, msg);
	});
}

void ZstMessageDispatcher::send_message(ZstMsgKind kind, bool async, MessageReceivedAction action)
{
	ZstMessage * msg = transport()->get_msg()->init_message(kind);
	send_to_stage(msg, async, action);
}

void ZstMessageDispatcher::send_message(ZstMsgKind kind, bool async, std::string msg_arg, MessageReceivedAction action)
{
	ZstMessage * msg = transport()->get_msg()->init_message(kind);
	msg->append_str(msg_arg.c_str(), msg_arg.size());
	send_to_stage(msg, async, action);
}

void ZstMessageDispatcher::send_message(ZstMsgKind kind, bool async, const std::vector<std::string> msg_args, MessageReceivedAction action)
{
	ZstMessage * msg = transport()->get_msg()->init_message(kind);
	for (auto s : msg_args) {
		msg->append_str(s.c_str(), s.size());
	}
	send_to_stage(msg, async, action);
}

void ZstMessageDispatcher::send_serialisable_message(ZstMsgKind kind, const ZstSerialisable & serialisable, bool async, MessageReceivedAction action)
{
	ZstMessage * msg = transport()->get_msg()->init_serialisable_message(kind, serialisable);
	send_to_stage(msg, async, action);
}

void ZstMessageDispatcher::send_serialisable_message(ZstMsgKind kind, const ZstSerialisable & serialisable, bool async, std::string msg_arg, MessageReceivedAction action)
{
	ZstMessage * msg = transport()->get_msg()->init_serialisable_message(kind, serialisable);
	msg->append_str(msg_arg.c_str(), msg_arg.size());
	send_to_stage(msg, async, action);
}

void ZstMessageDispatcher::send_serialisable_message(ZstMsgKind kind, const ZstSerialisable & serialisable, bool async, const std::vector<std::string> msg_args, MessageReceivedAction action)
{
	ZstMessage * msg = transport()->get_msg()->init_serialisable_message(kind, serialisable);
	for (auto s : msg_args) {
		msg->append_str(s.c_str(), s.size());
	}
	send_to_stage(msg, async, action);
}

void ZstMessageDispatcher::send_entity_message(const ZstEntityBase * entity, bool async, MessageReceivedAction action)
{
	ZstMessage * msg = transport()->get_msg()->init_entity_message(entity);
	send_to_stage(msg, async, action);
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

MessageFuture ZstMessageDispatcher::register_response_message(ZstMessage * msg)
{
	std::string id = std::string(msg->id());
	m_promise_messages[id] = MessagePromise();
	MessageFuture future = m_promise_messages[id].get_future();
	future = future.timeout(std::chrono::milliseconds(STAGE_TIMEOUT), ZstTimeoutException("Connect timeout"), m_timeout_watcher);
	return future;
}

void ZstMessageDispatcher::process_stage_response(ZstMessage * msg)
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
}

ZstTransportLayer * ZstMessageDispatcher::transport()
{
	assert(m_transport);
	return m_transport;
}
