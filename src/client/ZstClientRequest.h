#pragma once

#include <ZstMessage.h>
#include <ZstMessagePool.h>

class ZstClientRequest{
	ZstClientRequest();
	~ZstClientRequest();

	void send(ZstMessage * msg, bool async);
	ZstMsgKind send_sync(MessageFuture & future);
	ZstMsgKind send_async(MessageFuture & future);
	virtual void complete(ZstMsgKind status);
	virtual void failed(ZstMsgKind status);
}
