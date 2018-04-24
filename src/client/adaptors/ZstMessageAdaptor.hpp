#pragma once

#include <ZstExports.h>
#include <ZstConstants.h>
#include <entities/ZstPerformer.h>
#include <adaptors/ZstEventAdaptor.hpp>
#include "../core/ZstMessage.h"

class ZstMessageAdaptor : public ZstEventAdaptor {
public:
	ZST_CLIENT_EXPORT virtual ZstMessageReceipt on_send_to_stage(ZstStageMessage * msg, bool async, MessageBoundAction action);
	ZST_CLIENT_EXPORT virtual void on_receive_from_stage(int payload_index, ZstStageMessage * msg);
	ZST_CLIENT_EXPORT virtual void on_process_stage_response(ZstStageMessage * msg);

	ZST_CLIENT_EXPORT virtual void on_send_to_performance(ZstPerformanceMessage * msg);
	ZST_CLIENT_EXPORT virtual void on_receive_from_performance(ZstPerformanceMessage * msg);
};