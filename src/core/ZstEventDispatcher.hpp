#pragma once

#include <set>
#include <type_traits>
#include <tuple>
#include <unordered_map>
#include <functional>
#include <concurrentqueue.h>
#include <ZstLogging.h>
#include <adaptors/ZstEventAdaptor.hpp>

#include "ZstEventWakeup.hpp"

enum ZstEventStatus {
	FAILED = 0,
	SUCCESS
};


template<typename T>
class ZstEvent 
{
	static_assert(std::is_base_of<ZstEventAdaptor, std::remove_pointer_t<T> >::value, "T must derive from ZstEventAdaptor");
public:
	ZstEvent() : func([](T adp) {}), completed_func([](ZstEventStatus) {}) {}
	ZstEvent(std::function<void(T)> f, std::function<void(ZstEventStatus)> cf) : func(f), completed_func(cf) {};
	
	std::function<void(T)> func;
	std::function<void(ZstEventStatus)> completed_func;
};


template<typename T>
class ZstEventDispatcher
{
	static_assert(std::is_base_of<ZstEventAdaptor, std::remove_pointer_t<T> >::value, "T must derive from ZstEventAdaptor");
public:
	ZstEventDispatcher(const char * name = "") : 
		m_has_event(false),
		m_condition_wake(NULL)
	{
		m_name = std::string(name);
	}

	~ZstEventDispatcher() {
		this->flush();
		this->remove_all_adaptors();
	}

	void add_adaptor(T adaptor) { 
		this->m_adaptors.insert(adaptor); 
	}

	void set_wake_condition(std::shared_ptr<ZstEventWakeup> condition){
		this->m_condition_wake = condition;
	}

	void remove_adaptor(T adaptor) { 
		adaptor->set_target_dispatcher_inactive();
		this->m_adaptors.erase(adaptor); 
	}

	void remove_all_adaptors(){
		for (auto adp : m_adaptors) {
			adp->set_target_dispatcher_inactive();
		}
		m_adaptors.clear();
	}
	
	void flush() {
		ZstEvent<T> e;
		while (this->m_events.try_dequeue(e)) {}
	}

	void invoke(const std::function<void(T)> & event) {
		if (this->m_adaptors.size() < 1) {
			return;
		}
		for (T adaptor : this->m_adaptors) {
			event(adaptor);
		}
	}

	void defer(std::function<void(T)> event) {
		ZstEvent<T> e(event, [](ZstEventStatus s) {});
		this->m_events.enqueue(e);
		m_has_event = true;
		if(m_condition_wake)
			m_condition_wake->wake();
	}

	void defer(std::function<void(T)> event, std::function<void(ZstEventStatus)> on_complete) {
		ZstEvent<T> e(event, on_complete);
		this->m_events.enqueue(e);
		m_has_event = true;
		if(m_condition_wake)
			m_condition_wake->wake();
	}

	void process_events() {
		ZstEvent<T> event;

		while (this->m_events.try_dequeue(event)) {
			bool success = true;
			for (T adaptor : m_adaptors) {
				//try {
					event.func(adaptor);
				//}
				/*catch (std::exception e) {
					ZstLog::net(LogLevel::error, "Event dispatcher failed to run an event on a adaptor. Reason: {}", e.what());
					success = false;
				}*/
			}
			event.completed_func((success) ? ZstEventStatus::SUCCESS : ZstEventStatus::FAILED);
		}
		m_has_event = false;
	}

	bool has_event() {
		return m_has_event;
	}

private:
	std::set<T> m_adaptors;
	moodycamel::ConcurrentQueue< ZstEvent<T> > m_events;
	std::string m_name;
	bool m_has_event;
	std::shared_ptr<ZstEventWakeup> m_condition_wake;
};
