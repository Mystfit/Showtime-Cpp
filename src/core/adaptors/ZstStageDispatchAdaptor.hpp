#pragma once

#include <string>
#include <vector>
#include <functional>

#include <ZstExports.h>
#include <ZstConstants.h>

#include <ZstSerialisable.h>
#include <entities/ZstEntityBase.h>
#include <adaptors/ZstEventAdaptor.hpp>

#include "../core/ZstStageMessage.h"

typedef std::function<void(ZstMessageReceipt)> MessageReceivedAction;

class ZstStageDispatchAdaptor : public ZstEventAdaptor {
public:
	ZST_EXPORT virtual void send_message(ZstMsgKind kind, bool async, MessageReceivedAction action);
	ZST_EXPORT virtual void send_message(ZstMsgKind kind, bool async, std::string msg_arg, MessageReceivedAction action);
	ZST_EXPORT virtual void send_message(ZstMsgKind kind, bool async, const std::vector<std::string> msg_args, MessageReceivedAction action);

	ZST_EXPORT virtual void send_serialisable_message(ZstMsgKind kind, const ZstSerialisable & serialisable, bool async, MessageReceivedAction action);
	ZST_EXPORT virtual void send_serialisable_message(ZstMsgKind kind, const ZstSerialisable & serialisable, bool async, std::string msg_arg, MessageReceivedAction action);
	ZST_EXPORT virtual void send_serialisable_message(ZstMsgKind kind, const ZstSerialisable & serialisable, bool async, const std::vector<std::string> msg_args, MessageReceivedAction action);

	ZST_EXPORT virtual void send_entity_message(const ZstEntityBase * entity, bool async, MessageReceivedAction action);
	ZST_EXPORT virtual void on_receive_from_stage(ZstStageMessage * msg);
};
