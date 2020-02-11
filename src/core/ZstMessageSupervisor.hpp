#pragma once

#include <cf/cfuture.h>
#include <cf/time_watcher.h>
#include <concurrentqueue.h>
#include <unordered_map>
#include <boost/functional/hash.hpp>

#include "ZstExports.h"
#include "transports/ZstTransportHelpers.h"
#include "ZstMsgID.h"

namespace showtime {

typedef cf::promise<ZstMessageReceipt> ZstMessagePromise;
typedef cf::future<ZstMessageReceipt> ZstMessageFuture;

class ZstMessageSupervisor {
public:
    ZST_EXPORT ZstMessageSupervisor(std::shared_ptr<cf::time_watcher> timeout_watcher, long timeout_duration);
    ZST_EXPORT ~ZstMessageSupervisor();
    ZST_EXPORT void destroy();

    ZST_EXPORT ZstMessageFuture register_response(ZstMsgID id);
    ZST_EXPORT void enqueue_resolved_promise(ZstMsgID id);
    ZST_EXPORT void cleanup_response_messages();
    ZST_EXPORT void remove_response_promise(ZstMsgID id);
    ZST_EXPORT void process_response(ZstMsgID id, ZstMessageReceipt response);

private:
    std::unordered_map< ZstMsgID, ZstMessagePromise, boost::hash<boost::uuids::uuid> > m_response_promises;
    std::shared_ptr<cf::time_watcher> m_timeout_watcher;
    long m_timeout_duration;
    moodycamel::ConcurrentQueue<ZstMsgID> m_dead_promises;
};

}
