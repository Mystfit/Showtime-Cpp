#pragma once
#include <map>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <cf/cfuture.h>
#include <cf/time_watcher.h>

#include "../core/ZstMessage.h"
#include "../core/liasons/ZstPlugLiason.hpp"
#include "ZstTransportLayer.h"
#include "ZstClientModule.h"
#include "adaptors/ZstMessageAdaptor.hpp"


struct ZstTimeoutException : std::runtime_error {
	using std::runtime_error::runtime_error;
};

typedef cf::promise<ZstMsgKind> MessagePromise;
typedef cf::future<ZstMsgKind> MessageFuture;
typedef std::function<ZstMessage*()> MessageReceivedAction;

class ZstMessageDispatcher : 
	public ZstClientModule,
	public ZstMessageAdaptor,
	public ZstPlugLiason
{
public:

	ZstMessageDispatcher(ZstClient * client, ZstTransportLayer * transport);
	~ZstMessageDispatcher();

	ZstMessageReceipt send_to_stage(ZstStageMessage * msg, bool async, MessageBoundAction action);
	void send_to_performance(ZstPlug * plug);
	
	ZstStageMessage * init_entity_message(const ZstEntityBase * entity);
	ZstStageMessage * init_message(ZstMsgKind kind);
	ZstStageMessage * init_serialisable_message(ZstMsgKind kind, const ZstSerialisable & serialisable);
	ZstPerformanceMessage * init_performance_message(ZstPlug * plug);

private:
	ZstMessageDispatcher();

	ZstMessageReceipt send_sync_stage_message(ZstStageMessage * msg);
	ZstMessageReceipt send_async_stage_message(ZstStageMessage * msg, MessageBoundAction completed_action);

	virtual void complete(ZstMessageReceipt response);
	virtual void failed(ZstMessageReceipt status);

	MessageFuture register_response_message(ZstStageMessage * msg);
	void on_process_stage_response(ZstStageMessage * msg) override;

	std::unordered_map<std::string, MessagePromise > m_promise_messages;
	cf::time_watcher m_timeout_watcher;
	ZstTransportLayer * m_transport;
};
