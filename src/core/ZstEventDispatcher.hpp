#pragma once

#include <set>
#include <type_traits>
#include <tuple>
#include <unordered_map>
#include <functional>
#include <concurrentqueue.h>
#include "ZstLogging.h"
#include <mutex>
#include "adaptors/ZstEventAdaptor.hpp"

#include "ZstSemaphore.h"

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
		m_has_event(false)
	{
		m_name = std::string(name);
	}

	~ZstEventDispatcher() noexcept{
        this->remove_all_adaptors();
	}

	void add_adaptor(T adaptor) { 
		std::lock_guard<std::recursive_mutex> lock(m_mtx);
		this->m_adaptors.insert(adaptor);
	}

	void remove_adaptor(T adaptor) { 
		std::lock_guard<std::recursive_mutex> lock(m_mtx);
		adaptor->set_target_dispatcher_inactive();
		this->m_adaptors.erase(adaptor); 
	}
	
	size_t num_adaptors() {
		return m_adaptors.size();
	}

	void remove_all_adaptors(){
		std::lock_guard<std::recursive_mutex> lock(m_mtx);
		for (auto adp : m_adaptors) {
			adp->set_target_dispatcher_inactive();
		}
		m_adaptors.clear();
	}

	void set_wake_condition(std::weak_ptr<ZstSemaphore> condition) {
		//std::lock_guard<std::recursive_mutex> lock(m_mtx);
		this->m_condition_wake = condition;
	}
	
	void flush() {
		ZstEvent<T> e;
        //std::lock_guard<std::recursive_mutex> lock(m_mtx);
		while (this->m_events.try_dequeue(e)) {}
	}

	void invoke(const std::function<void(T)> & event) {
		if (this->m_adaptors.size() < 1) {
			return;
		}

		std::lock_guard<std::recursive_mutex> lock(m_mtx);
		for (T adaptor : this->m_adaptors) {
            if(adaptor){
                event(adaptor);
            }
		}
	}

	void defer(std::function<void(T)> event) {
		ZstEvent<T> e(event, [](ZstEventStatus s) {});
		this->m_events.enqueue(e);
		m_has_event = true;
		if(auto shared = m_condition_wake.lock())
            shared->notify();
	}

	void defer(std::function<void(T)> event, std::function<void(ZstEventStatus)> on_complete) {
		ZstEvent<T> e(event, on_complete);
		this->m_events.enqueue(e);
		m_has_event = true;
        if(auto shared = m_condition_wake.lock())
            shared->notify();
	}

	void process_events() {
		ZstEvent<T> event;

		while (this->m_events.try_dequeue(event)) {
			bool success = true;

			std::lock_guard<std::recursive_mutex> lock(m_mtx);
			for (T adaptor : m_adaptors) {
				event.func(adaptor);
				/*try {
					event.func(adaptor);
				}
				catch (std::exception e) {
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
	std::weak_ptr<ZstSemaphore> m_condition_wake;
	std::recursive_mutex m_mtx;
};
