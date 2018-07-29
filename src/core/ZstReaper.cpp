#include <ZstLogging.h>
#include "ZstReaper.h"

void ZstReaper::add(ZstSynchronisable * synchronisable)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	ZstLog::net(LogLevel::debug, "Adding synchronisable with id:{} to reaper", synchronisable->instance_id());
	m_items_to_reap[ZstURI()] = synchronisable;
	lock.unlock();
}

void ZstReaper::add(ZstEntityBase * entity)
{
	bool should_add = true;
	
	std::unique_lock<std::mutex> lock(m_mutex);

	auto it = m_items_to_reap.begin();
	while (it != m_items_to_reap.end()) {
		if (!it->first.is_empty() && it->first.contains(entity->URI())) {
			//Don't add, a parent entity is going to be leaving and will take this item with it
			should_add = false;
			break;
		}

		if (entity->URI().contains(it->first)) {
			//Remove queued entity, new item will prune it
			m_items_to_reap.erase(it->first);
		}
		else {
			++it;
		}
	}

	if (should_add) {
		ZstLog::net(LogLevel::debug, "Adding entity {} with id:{} to reaper", entity->URI().path(), entity->instance_id());
		m_items_to_reap[entity->URI()] = entity;
	}

	lock.unlock();
}

void ZstReaper::reap_all()
{
	std::unique_lock<std::mutex> lock(m_mutex);

	if (m_items_to_reap.size()) {
		for (auto synchronisable : m_items_to_reap) {
			ZstLog::net(LogLevel::debug, "Reaper removing instance {}", synchronisable.second->instance_id());
			delete synchronisable.second;
		}
		m_items_to_reap.clear();
	}

	lock.unlock();
}
