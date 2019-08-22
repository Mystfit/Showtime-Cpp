#pragma once

#include <cf/cfuture.h>
#include <cf/time_watcher.h>
#include <concurrentqueue.h>
#include "ZstExports.h"
#include "ZstMessageOptions.h"
#include "transports/ZstTransportHelpers.h"
#include "ZstMsgID.h"

using namespace cf;
using namespace moodycamel;

typedef promise<ZstMessageReceipt> ZstMessagePromise;
typedef future<ZstMessageReceipt> ZstMessageFuture;

class ZstMessageSupervisor {
public:
	ZST_EXPORT ZstMessageSupervisor(std::shared_ptr<time_watcher> timeout_watcher, long timeout_duration);
	ZST_EXPORT ~ZstMessageSupervisor();
    ZST_EXPORT void destroy();

	ZST_EXPORT ZstMessageFuture register_response(ZstMsgID id);
	ZST_EXPORT void enqueue_resolved_promise(ZstMsgID id);
	ZST_EXPORT void cleanup_response_messages();
	ZST_EXPORT void remove_response_promise(ZstMsgID id);
	ZST_EXPORT void process_response(ZstMsgID id, ZstMessageReceipt response);

private:
	std::unordered_map< ZstMsgID, ZstMessagePromise > m_response_promises;
	std::shared_ptr<time_watcher> m_timeout_watcher;
	long m_timeout_duration;
	ConcurrentQueue<ZstMsgID> m_dead_promises;
};
