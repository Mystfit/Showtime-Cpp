#pragma once

#include <future>

#include <concurrentqueue.h>
#include <unordered_map>
#include <boost/functional/hash.hpp>
#include <mutex>

#include "ZstEventDispatcher.hpp"
#include "ZstExports.h"
#include "transports/ZstTransportHelpers.h"
#include "ZstMsgID.h"
#include "ZstMessage.h"

namespace showtime {

typedef std::promise< std::shared_ptr<ZstMessage> > ZstMessagePromise;
typedef std::future< std::shared_ptr<ZstMessage> > ZstMessageFuture;

class ZstMessageSupervisor {
public:
    ZST_EXPORT void destroy();

    ZST_EXPORT ZstMessageFuture register_response(ZstMsgID id);
    ZST_EXPORT void enqueue_resolved_promise(ZstMsgID id);
    ZST_EXPORT void cleanup_response_messages();

    ZST_EXPORT void take_message_ownership(std::shared_ptr<ZstMessage>& msg, ZstEventCallback cleanup_func);
    ZST_EXPORT void release_owned_message(std::shared_ptr<ZstMessage>& msg);

    ZST_EXPORT void remove_response_promise(ZstMsgID id);
    ZST_EXPORT std::promise< std::shared_ptr<ZstMessage> >& get_response_promise(std::shared_ptr<ZstMessage> message);

private:
    std::unordered_map< ZstMsgID, ZstMessagePromise, boost::hash<boost::uuids::uuid> > m_response_promises;
    std::unordered_map<ZstMsgID, std::pair<std::shared_ptr<ZstMessage>, ZstEventCallback>, boost::hash<boost::uuids::uuid> > m_owned_messages;
    moodycamel::ConcurrentQueue<ZstMsgID> m_dead_promises;
    std::mutex m_mtx;
};

}
