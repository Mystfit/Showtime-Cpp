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

#include <ZstEventDispatcher.hpp>
#include "adaptors/ZstStageDispatchAdaptor.hpp"
#include "adaptors/ZstPerformanceDispatchAdaptor.hpp"

struct ZstTimeoutException : std::runtime_error {
	using std::runtime_error::runtime_error;
};

typedef cf::promise<ZstMsgKind> MessagePromise;
typedef cf::future<ZstMsgKind> MessageFuture;

class ZstMessageDispatcher : 
	public ZstEventDispatcher<ZstStageDispatchAdaptor*>,
	public ZstEventDispatcher<ZstPerformanceDispatchAdaptor*>,
	public ZstStageDispatchAdaptor,
	public ZstPerformanceDispatchAdaptor,
	public ZstClientModule,
	public ZstPlugLiason
{
	using ZstEventDispatcher<ZstStageDispatchAdaptor*>::run_event;
	using ZstEventDispatcher<ZstStageDispatchAdaptor*>::add_adaptor;
	using ZstEventDispatcher<ZstPerformanceDispatchAdaptor*>::run_event;
	using ZstEventDispatcher<ZstPerformanceDispatchAdaptor*>::add_adaptor;

public:
	ZstMessageDispatcher();
	~ZstMessageDispatcher();
	void set_transport(ZstTransportLayer * transport);

	void init() override {};
	void destroy() override {};

	void process_events();

	void send_to_stage(ZstMessage * msg, bool async, MessageReceivedAction action);
	void send_to_performance(ZstPlug * plug);

	void receive_from_stage(size_t payload_index, ZstMessage * msg);
	void receive_from_performance(ZstMessage * msg);

	void send_message(ZstMsgKind kind, bool async, MessageReceivedAction action) override;
	void send_message(ZstMsgKind kind, bool async, std::string msg_arg, MessageReceivedAction action) override;
	void send_message(ZstMsgKind kind, bool async, const std::vector<std::string> msg_args, MessageReceivedAction action) override;
	void send_serialisable_message(ZstMsgKind kind, const ZstSerialisable & serialisable, bool async, MessageReceivedAction action) override;
	void send_serialisable_message(ZstMsgKind kind, const ZstSerialisable & serialisable, bool async, std::string msg_arg, MessageReceivedAction action) override;
	void send_serialisable_message(ZstMsgKind kind, const ZstSerialisable & serialisable, bool async, const std::vector<std::string> msg_args, MessageReceivedAction action) override;
	void send_entity_message(const ZstEntityBase * entity, bool async, MessageReceivedAction action) override;

	void process_stage_response(ZstMessage * msg);

private:

	ZstMessageReceipt send_sync_stage_message(ZstMessage * msg);
	void send_async_stage_message(ZstMessage * msg, MessageReceivedAction completed_action);
	
	virtual void complete(ZstMessageReceipt response);
	virtual void failed(ZstMessageReceipt status);

	MessageFuture register_response_message(ZstMessage * msg);

	std::unordered_map<std::string, MessagePromise > m_promise_messages;
	cf::time_watcher m_timeout_watcher;

	ZstTransportLayer * transport();
	ZstTransportLayer * m_transport;
};
