#pragma once
#include <showtime/ZstPointerUtils.h>
#include <showtime/ZstExports.h>
#include <showtime/multicast.hpp>
#include <set>
#include <memory>
#include <mutex>


namespace showtime {

#define MULTICAST_DELEGATE_BODY(EventName)\
	EventName & On##EventName##Event() { return m_##EventName;};\
private:\
	EventName m_##EventName;\
public:
#define MULTICAST_DELEGATE(EventName) typedef util::multifunction<void()> EventName; MULTICAST_DELEGATE_BODY(EventName)
#define MULTICAST_DELEGATE_OneParam(EventName, Arg1) typedef util::multifunction<void(Arg1)> EventName; MULTICAST_DELEGATE_BODY(EventName)
#define MULTICAST_DELEGATE_TwoParams(EventName, Arg1, Arg2) typedef util::multifunction<void(Arg1, Arg2)> EventName; MULTICAST_DELEGATE_BODY(EventName)



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
