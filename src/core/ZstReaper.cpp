#include <ZstLogging.h>
#include "ZstReaper.h"

void ZstReaper::add(ZstSynchronisable * synchronisable)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	ZstLog::net(LogLevel::debug, "Adding synchronisable with id:{} to reaper", synchronisable->instance_id());
	m_items_to_reap.insert(synchronisable);
	lock.unlock();
}

void ZstReaper::reap_all()
{
	std::unique_lock<std::mutex> lock(m_mutex);

	if (m_items_to_reap.size()) {
		for (auto synchronisable : m_items_to_reap) {
			ZstLog::net(LogLevel::debug, "Reaper removing instance {}", synchronisable->instance_id());
			delete synchronisable;
		}
		m_items_to_reap.clear();
	}

	lock.unlock();
}
