#pragma once
#include "ZstLogging.h"
#include "ZstPointerUtils.h"
#include <set>
#include <memory>
#include <mutex>

namespace showtime {

//Forwards
class ZstEventDispatcherBase;

class ZST_CLASS_EXPORTED ZstEventAdaptor : public inheritable_enable_shared_from_this<ZstEventAdaptor>
{
    friend class ZstEventDispatcherBase;
public:
	ZST_EXPORT ZstEventAdaptor();
	ZST_EXPORT virtual ~ZstEventAdaptor();

	ZST_EXPORT bool contains_event_source(std::weak_ptr<ZstEventDispatcherBase> event_source);
	ZST_EXPORT void prune_dispatchers();

private:
	ZST_EXPORT void add_event_source(std::weak_ptr<ZstEventDispatcherBase> event_source);
	ZST_EXPORT void remove_event_source(std::weak_ptr<ZstEventDispatcherBase> event_source);

	std::set< std::weak_ptr<ZstEventDispatcherBase>, std::owner_less<std::weak_ptr<ZstEventDispatcherBase> > > m_event_sources;

	std::recursive_mutex m_mtx;
};

}
