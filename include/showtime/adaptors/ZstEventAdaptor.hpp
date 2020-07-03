#pragma once
#include <showtime/ZstPointerUtils.h>
#include <showtime/ZstExports.h>
#include <showtime/multicast.hpp>
#include <set>
#include <memory>
#include <mutex>


namespace showtime {

#define MULTICAST_DELEGATE_EVENT_BODY(EventName) EventName##_delegate & on_##EventName##_event() { return m_##EventName;};\
private:\
	EventName##_delegate m_##EventName;\
public:

#define MULTICAST_DELEGATE(ExportPrefix, EventName) ExportPrefix virtual void on_##EventName()##_event {};\
typedef util::multifunction<void()> EventName;\
ExportPrefix MULTICAST_DELEGATE_EVENT_BODY(EventName)

#define MULTICAST_DELEGATE_OneParam(ExportPrefix, EventName,  Arg1Type, Arg1Name) ExportPrefix virtual void on_##EventName##(Arg1Type Arg1Name){};\
typedef util::multifunction<void(Arg1Type)> EventName##_delegate;\
ExportPrefix MULTICAST_DELEGATE_EVENT_BODY(EventName)

#define MULTICAST_DELEGATE_TwoParams(ExportPrefix, EventName, Arg1Type, Arg1Name, Arg2Type, Arg2Name) ExportPrefix virtual void on_##EventName##(Arg1Type Arg1Name, Arg2Type Arg2Name) {};\
typedef util::multifunction<void(Arg1Type, Arg2Type)> EventName##_delegate;\
ExportPrefix MULTICAST_DELEGATE_EVENT_BODY(EventName)


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
