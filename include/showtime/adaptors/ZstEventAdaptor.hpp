#pragma once
#include "ZstLogging.h"
#include <vector>
#include <mutex>

//Forwards
class ZstEventDispatcherBase;

class ZST_EXPORT ZstEventAdaptor {
	friend class ZstEventDispatcherBase;
public:
	ZST_EXPORT ZstEventAdaptor();
	ZST_EXPORT virtual ~ZstEventAdaptor();

	ZST_EXPORT bool is_target_dispatcher_active();
	ZST_EXPORT void set_target_dispatcher_inactive();

private:
	ZST_EXPORT void add_event_source(ZstEventDispatcherBase* event_source);
	ZST_EXPORT void remove_event_source(ZstEventDispatcherBase* event_source);

	bool m_is_target_dispatcher_active;
	std::vector<ZstEventDispatcherBase*> m_event_sources;

	std::recursive_mutex m_mtx;
};
