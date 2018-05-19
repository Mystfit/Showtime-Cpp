#pragma once
#include <map>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <cf/cfuture.h>
#include <cf/time_watcher.h>

#include <ZstEventDispatcher.hpp>

#include "../core/ZstStageMessage.h"
#include "../core/ZstPerformanceMessage.h"
#include "../core/liasons/ZstPlugLiason.hpp"
#include "../core/adaptors/ZstStageDispatchAdaptor.hpp"
#include "../core/adaptors/ZstPerformanceDispatchAdaptor.hpp"

#include "ZstTransportLayer.h"
#include "ZstClientModule.h"


struct ZstTimeoutException : std::runtime_error {
	using std::runtime_error::runtime_error;
};

typedef cf::promise<ZstMsgKind> MessagePromise;
typedef cf::future<ZstMsgKind> MessageFuture;

class ZstMessageDispatcher : 
	public ZstStageDispatchAdaptor,
	public ZstPerformanceDispatchAdaptor,
	public ZstClientModule,
	public ZstPlugLiason
{
public:
	ZstMessageDispatcher();
	~ZstMessageDispatcher();
	void set_transport(ZstTransportLayer * transport);

	void init() override {};
	void destroy() override {};

	void process_events();
	void flush_events();

	void send_to_stage(ZstStageMessage * msg, bool async, MessageReceivedAction action);
	void send_to_performance(ZstOutputPlug * plug) override;

	void receive_addressed_msg(size_t payload_index, ZstStageMessage * msg);
	void receive_from_performance(ZstPerformanceMessage * msg);

	void send_message(ZstMsgKind kind, bool async, MessageReceivedAction action) override;
	void send_message(ZstMsgKind kind, bool async, std::string msg_arg, MessageReceivedAction action) override;
	void send_message(ZstMsgKind kind, bool async, const std::vector<std::string> msg_args, MessageReceivedAction action) override;
	void send_serialisable_message(ZstMsgKind kind, const ZstSerialisable & serialisable, bool async, MessageReceivedAction action) override;
	void send_serialisable_message(ZstMsgKind kind, const ZstSerialisable & serialisable, bool async, std::string msg_arg, MessageReceivedAction action) override;
	void send_serialisable_message(ZstMsgKind kind, const ZstSerialisable & serialisable, bool async, const std::vector<std::string> msg_args, MessageReceivedAction action) override;
	void send_entity_message(const ZstEntityBase * entity, bool async, MessageReceivedAction action) override;

	void process_stage_response(ZstStageMessage * msg);

	//Adaptors
	ZstEventDispatcher<ZstStageDispatchAdaptor*> & stage_events();
	ZstEventDispatcher<ZstPerformanceDispatchAdaptor*> & performance_events();

private:

	ZstMessageReceipt send_sync_stage_message(ZstStageMessage * msg);
	void send_async_stage_message(ZstStageMessage * msg, MessageReceivedAction completed_action);
	
	virtual void complete(ZstMessageReceipt response);
	virtual void failed(ZstMessageReceipt status);

	MessageFuture register_response_message(ZstStageMessage * msg);

	std::unordered_map<std::string, MessagePromise > m_promise_messages;
	cf::time_watcher m_timeout_watcher;

	ZstTransportLayer * transport();
	ZstTransportLayer * m_transport;

	ZstEventDispatcher<ZstStageDispatchAdaptor*> m_stage_events;
	ZstEventDispatcher<ZstPerformanceDispatchAdaptor*> m_performance_events;
};
