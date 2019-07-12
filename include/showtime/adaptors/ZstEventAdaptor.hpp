#pragma once
#include "ZstLogging.h"
#include <vector>
#include <mutex>

//Forwards
class ZstEventDispatcherBase;

class ZstEventAdaptor {
	friend class ZstEventDispatcherBase;
public:
	ZstEventAdaptor();
	virtual ~ZstEventAdaptor();

	bool is_target_dispatcher_active();
	void set_target_dispatcher_inactive();

private:
	void add_event_source(ZstEventDispatcherBase* event_source);
	void remove_event_source(ZstEventDispatcherBase* event_source);

	bool m_is_target_dispatcher_active;
	std::vector<ZstEventDispatcherBase*> m_event_sources;

	std::recursive_mutex m_mtx;
};
