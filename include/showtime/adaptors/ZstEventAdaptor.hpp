#pragma once
#include "ZstLogging.h"
#include "ZstPointerUtils.h"
#include <unordered_set>
#include <memory>
#include <mutex>


//Forwards
class ZstEventDispatcherBase;


class ZST_CLASS_EXPORTED ZstEventAdaptor : public inheritable_enable_shared_from_this<ZstEventAdaptor>
{
	friend class ZstEventDispatcherBase;
public:
	ZST_EXPORT ZstEventAdaptor();
	ZST_EXPORT virtual ~ZstEventAdaptor();

	ZST_EXPORT bool is_target_dispatcher_active();
	ZST_EXPORT void set_target_dispatcher_inactive();

private:
	ZST_EXPORT void add_event_source(std::shared_ptr<ZstEventDispatcherBase> event_source);
	ZST_EXPORT void remove_event_source(std::shared_ptr<ZstEventDispatcherBase> event_source);

	bool m_is_target_dispatcher_active;
	std::unordered_set< std::shared_ptr<ZstEventDispatcherBase> > m_event_sources;

	std::recursive_mutex m_mtx;
};
