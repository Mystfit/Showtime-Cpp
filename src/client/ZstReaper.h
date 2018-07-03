#pragma once

#include <unordered_map>
#include <mutex>
#include <entities/ZstEntityBase.h>

class ZstReaper {
public:
	void add(ZstSynchronisable * synchronisable);
	void add(ZstEntityBase * entity);
	void reap_all();
private:
	std::mutex m_mutex;
	std::unordered_map<ZstURI, ZstSynchronisable*, ZstURIHash> m_items_to_reap;
};
