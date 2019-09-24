#include "ZstTransportLayerBase.hpp"
#include "../adaptors/ZstTransportAdaptor.hpp"
#include "ZstConstants.h"
#include "ZstExceptions.h"

namespace showtime {


ZstMessageReceipt ZstTransportLayerBase::begin_send_message(ZstMessage * msg)
{
	send_message_impl(msg, ZstTransportArgs());
	return ZstMessageReceipt{ ZstMsgKind::OK, ZstTransportRequestBehaviour::PUBLISH };
}

ZstMessageReceipt ZstTransportLayerBase::begin_send_message(ZstMessage * msg, const ZstTransportArgs& args)
{
	switch (args.msg_send_behaviour) {
	case ZstTransportRequestBehaviour::ASYNC_REPLY:
		return send_async_message(msg, args);
		break;
	case ZstTransportRequestBehaviour::SYNC_REPLY:
	{
		auto receipt = send_sync_message(msg, args);
		args.on_recv_response(receipt);
		return receipt;
		break;
	}
	case ZstTransportRequestBehaviour::PUBLISH: {
		send_message_impl(msg, args);
		return ZstMessageReceipt{ ZstMsgKind::OK, ZstTransportRequestBehaviour::PUBLISH };
		break;
	}
	default:
		ZstLog::net(LogLevel::error, "Can't send message. Unknown message request behaviour");
		break;
	}
	return ZstMessageReceipt{ ZstMsgKind::EMPTY };
}

}
