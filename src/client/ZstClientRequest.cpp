#include "ZstClientRequest.h"

void ZstClientRequest::send(ZstMessage * msg, bool async) {
	//Build connect message
	
	MessageFuture future = m_msg_pool.register_future(msg, true);
	if(async){
		register_client_to_stage_async(future);
		send_to_stage(msg);
	}
	else {
		send_to_stage(msg);
		register_client_to_stage_sync(future);
	}
}

ZstMsgKind ZstClientRequest::send_sync(MessageFuture & future)
{
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

ZstMsgKind ZstClientRequest::send_async(MessageFuture & future)
{
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

void ZstClientRequest::complete(ZstMsgKind status)
{
	//If we didn't receive a OK signal, something went wrong
	if (status != ZstMsgKind::OK) {
        ZstLog::net(LogLevel::error, "Stage connection failed with with status: {}", status);
        return;
	}
}

void ZstClientRequest::complete(ZstMsgKind status)
{
	//If we didn't receive a OK signal, something went wrong
	if (status != ZstMsgKind::OK) {
        ZstLog::net(LogLevel::error, "Stage connection failed with with status: {}", status);
        return;
	}
}