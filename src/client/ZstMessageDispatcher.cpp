#include "ZstMessageDispatcher.h"

ZstMessageDispatcher::ZstMessageDispatcher()
{	
	m_message_pool.populate(MESSAGE_POOL_BLOCK);
}

ZstMessageDispatcher::~ZstMessageDispatcher()
{
}

ZstMsgKind ZstMessageDispatcher::prepare_sync_message(const ZstMessage * msg)
{
	if (!msg) return ZstMsgKind::EMPTY;

	MessageFuture future = register_response_message(msg);
	ZstMsgKind status(ZstMsgKind::EMPTY);
	try {
		status = future.get();
		complete(status);
		process_callbacks();
	}
	catch (const ZstTimeoutException & e) {
		ZstLog::net(LogLevel::error, "Server timed out", e.what());
		status = ZstMsgKind::ERR_STAGE_TIMEOUT;
		failed(status);
	}
	return status;
}

void ZstMessageDispatcher::prepare_async_message(const ZstMessage * msg)
{
	if (!msg) return;
	
	MessageFuture future = register_response_message(msg);
	future.then([this](MessageFuture f) {
		ZstMsgKind status(ZstMsgKind::EMPTY);
		try {
			status = f.get();
			complete(status);
		}
		catch (const ZstTimeoutException & e) {
			ZstLog::net(LogLevel::error, "Stage async join timed out - {}", e.what());
			status = ZstMsgKind::ERR_STAGE_TIMEOUT;
			failed(status);
		}
		return status;
	});
}

void ZstMessageDispatcher::complete(ZstMsgKind status)
{
	//If we didn't receive a OK signal, something went wrong
	if (status != ZstMsgKind::OK) {
        ZstLog::net(LogLevel::error, "Stage connection failed with with status: {}", status);
        return;
	}
}

void ZstMessageDispatcher::failed(ZstMsgKind status)
{
}

ZstMessagePool & ZstMessageDispatcher::msg_pool()
{
	return m_message_pool;
}

void ZstMessageDispatcher::complete(ZstMsgKind status)
{
	//If we didn't receive a OK signal, something went wrong
	if (status != ZstMsgKind::OK) {
        ZstLog::net(LogLevel::error, "Stage connection failed with with status: {}", status);
        return;
	}
}


MessageFuture ZstMessageDispatcher::register_response_message(const ZstMessage * msg);
{
	std::string id = std::string(msg->id());
	m_promise_messages[id] = MessagePromise();
	MessageFuture future = m_promise_messages[id].get_future();
	future = future.timeout(std::chrono::milliseconds(STAGE_TIMEOUT), ZstTimeoutException("Connect timeout"), m_timeout_watcher);
	return future;
}

int ZstMessageDispatcher::process_response_message(const ZstMessage * msg)
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