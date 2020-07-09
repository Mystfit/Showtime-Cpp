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
	//friend class ZstEventDispatcherTyped<T>;
public:
	ZstEventAdaptor() {}
	virtual ~ZstEventAdaptor() {
		//auto sources = m_event_sources;
		for (auto source : m_event_sources) {
			if (auto src = source.lock())
				src->prune_missing_adaptors();
		}
		//m_event_sources.clear();
	}

	bool contains_event_source(std::weak_ptr< IEventDispatcher > event_source) {
		std::lock_guard<std::recursive_mutex> lock(m_mtx);
		return (m_event_sources.find(event_source) != m_event_sources.end()) ? true : false;
	}

	void prune_dispatchers() {
		std::lock_guard<std::recursive_mutex> lock(m_mtx);
		auto sources = m_event_sources;
		for (auto src : sources) {
			if (src.expired())
				m_event_sources.erase(src);
		}
	}

	void add_event_source(std::weak_ptr< IEventDispatcher > event_source) {
		std::lock_guard<std::recursive_mutex> lock(m_mtx);
		m_event_sources.insert(event_source);
	}

	void remove_event_source(std::weak_ptr< IEventDispatcher > event_source) {
		std::lock_guard<std::recursive_mutex> lock(m_mtx);
		m_event_sources.erase(event_source);
	}

private:
	std::set< std::weak_ptr< IEventDispatcher >, std::owner_less< std::weak_ptr< IEventDispatcher > > > m_event_sources;
	std::recursive_mutex m_mtx;
};
}
