#pragma once
#include <showtime/ZstPointerUtils.h>
#include <showtime/ZstExports.h>
#include <showtime/multicast.hpp>
#include <showtime/ZstEventDispatcher.h>
#include <set>
#include <memory>
#include <mutex>


namespace showtime {

#define MULTICAST_DELEGATE_EVENT_BODY(EventName) EventName##_delegate & EventName() { return m_##EventName;};\
private:\
	EventName##_delegate m_##EventName;\
public:

#define MULTICAST_DELEGATE(ExportPrefix, EventName) ExportPrefix virtual void on_##EventName() { m_##EventName(); };\
typedef util::multifunction<void()> EventName;\
ExportPrefix MULTICAST_DELEGATE_EVENT_BODY(EventName)

#define MULTICAST_DELEGATE_OneParam(ExportPrefix, EventName,  Arg1Type, Arg1Name) ExportPrefix virtual void on_##EventName(Arg1Type Arg1Name){ m_##EventName(Arg1Name); };\
typedef util::multifunction<void(Arg1Type)> EventName##_delegate;\
ExportPrefix MULTICAST_DELEGATE_EVENT_BODY(EventName)

#define MULTICAST_DELEGATE_TwoParams(ExportPrefix, EventName, Arg1Type, Arg1Name, Arg2Type, Arg2Name) ExportPrefix virtual void on_##EventName(Arg1Type Arg1Name, Arg2Type Arg2Name) { m_##EventName(Arg1Name, Arg2Name); };\
typedef util::multifunction<void(Arg1Type, Arg2Type)> EventName##_delegate;\
ExportPrefix MULTICAST_DELEGATE_EVENT_BODY(EventName)


template<typename T>
class ZST_CLASS_EXPORTED ZstEventAdaptor : public inheritable_enable_shared_from_this< ZstEventAdaptor<T> >
{
public:
private:
};
}
