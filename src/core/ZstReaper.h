#pragma once

#include <unordered_map>
#include <mutex>
#include <functional>
#include <concurrentqueue.h>

#include "entities/ZstEntityBase.h"
#include "ZstExports.h"

namespace showtime {

class ZstReaper {
public:
	ZST_EXPORT void add(ZstSynchronisable * synchronisable);
    ZST_EXPORT void add_cleanup_op(std::function<void()> cleanup_cb);
	ZST_EXPORT void reap_all();
private:
	std::mutex m_mutex;
	std::unordered_set<ZstSynchronisable*> m_items_to_reap;
    moodycamel::ConcurrentQueue< std::function<void()> > m_cleanup_callbacks;
};

}
