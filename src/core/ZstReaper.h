#pragma once

#include <unordered_map>
#include <mutex>
#include <entities/ZstEntityBase.h>
#include <ZstExports.h>

class ZstReaper {
public:
	ZST_EXPORT void add(ZstSynchronisable * synchronisable);
	ZST_EXPORT void reap_all();
private:
	std::mutex m_mutex;
	std::unordered_set<ZstSynchronisable*> m_items_to_reap;
};
