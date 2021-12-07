#pragma once
#include <showtime/ZstPointerUtils.h>
#include <showtime/ZstExports.h>
#include <showtime/multicast.hpp>
#include <showtime/ZstEventDispatcher.h>
#include <set>
#include <memory>
#include <mutex>


namespace showtime {

#define MULTICAST_DELEGATE_EVENT_BODY(EventName) EventName##_delegate * EventName() {\
	return m_##EventName.get();\
};\
private:\
	std::shared_ptr<EventName##_delegate> m_##EventName;\
public:

#define MULTICAST_DELEGATE(ExportPrefix, EventName) ExportPrefix virtual void on_##EventName() { m_##EventName->operator()(); };\
typedef util::multifunction<void()> EventName;\
ExportPrefix MULTICAST_DELEGATE_EVENT_BODY(EventName)

#define MULTICAST_DELEGATE_OneParam(ExportPrefix, EventName,  Arg1Type, Arg1Name) ExportPrefix virtual void on_##EventName(Arg1Type Arg1Name){ m_##EventName->operator()(Arg1Name); };\
typedef util::multifunction<void(Arg1Type)> EventName##_delegate;\
ExportPrefix MULTICAST_DELEGATE_EVENT_BODY(EventName)

#define MULTICAST_DELEGATE_TwoParams(ExportPrefix, EventName, Arg1Type, Arg1Name, Arg2Type, Arg2Name) ExportPrefix virtual void on_##EventName(Arg1Type Arg1Name, Arg2Type Arg2Name) { m_##EventName->operator()(Arg1Name, Arg2Name); };\
typedef util::multifunction<void(Arg1Type, Arg2Type)> EventName##_delegate;\
ExportPrefix MULTICAST_DELEGATE_EVENT_BODY(EventName)

#define MULTICAST_DELEGATE_INITIALIZER(EventName) m_##EventName = std::make_shared<EventName##_delegate>();

template<typename T>
class ZST_CLASS_EXPORTED ZstEventAdaptor : public inheritable_enable_shared_from_this< ZstEventAdaptor<T> >
{
public:
private:
};
}
