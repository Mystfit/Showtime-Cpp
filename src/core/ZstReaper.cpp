#include "ZstLogging.h"
#include "ZstReaper.h"

namespace showtime {

void ZstReaper::add(ZstSynchronisable * synchronisable)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	ZstLog::net(LogLevel::debug, "Adding synchronisable with id:{} to reaper", synchronisable->instance_id());
	m_items_to_reap.insert(synchronisable);
}

void ZstReaper::add_cleanup_op(std::function<void()> cleanup_cb)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cleanup_callbacks.enqueue(cleanup_cb);
}

void ZstReaper::reap_all()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Process cleanup callbacks
    std::function<void()> cb;
    while (this->m_cleanup_callbacks.try_dequeue(cb)) {
        cb();
    }
    
    // Remove objects
    for (auto synchronisable : m_items_to_reap) {
        //ZstLog::net(LogLevel::debug, "Reaper removing instance {}", synchronisable->instance_id());
        delete synchronisable;
    }
    
    m_items_to_reap.clear();
}

}
