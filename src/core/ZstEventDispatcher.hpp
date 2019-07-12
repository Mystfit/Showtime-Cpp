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


class ZstEventDispatcherBase {
public:
	ZstEventDispatcherBase() : m_has_event(false)
	{
	}

	ZstEventDispatcherBase(const char* name) : 
		m_name(name),
		m_has_event(false)
	{
	}

	~ZstEventDispatcherBase() noexcept {
		this->remove_all_adaptors();
	}

	void add_adaptor(ZstEventAdaptor * adaptor) {
		std::lock_guard<std::recursive_mutex> lock(m_mtx);
		this->m_adaptors.insert(adaptor);
		adaptor->add_event_source(this);
	}

	void remove_adaptor(ZstEventAdaptor * adaptor) {
		std::lock_guard<std::recursive_mutex> lock(m_mtx);
		adaptor->set_target_dispatcher_inactive();
		this->m_adaptors.erase(adaptor);
		adaptor->remove_event_source(this);
	}

	size_t num_adaptors() {
		return m_adaptors.size();
	}

	void remove_all_adaptors() {
		std::lock_guard<std::recursive_mutex> lock(m_mtx);
		auto adaptors = m_adaptors;
		for (auto adp : adaptors) {
			remove_adaptor(adp);
		}
		m_adaptors.clear();
	}

	void set_wake_condition(std::weak_ptr<ZstSemaphore> condition) {
		//std::lock_guard<std::recursive_mutex> lock(m_mtx);
		this->m_condition_wake = condition;
	}

	bool has_event() {
		return m_has_event;
	}

protected:
	std::set<ZstEventAdaptor*> m_adaptors;
	std::weak_ptr<ZstSemaphore> m_condition_wake;
	std::recursive_mutex m_mtx;
	bool m_has_event;
private:
	std::string m_name;
};


template<typename T>
class ZstEventDispatcher : public ZstEventDispatcherBase
{
	static_assert(std::is_base_of<ZstEventAdaptor, std::remove_pointer_t<T> >::value, "T must derive from ZstEventAdaptor");
public:
	ZstEventDispatcher() :
		ZstEventDispatcherBase("")
	{
	}

	ZstEventDispatcher(const char* name) :
		ZstEventDispatcherBase(name)
	{
	}

	void flush() {
		ZstEvent<T> e;
		//std::lock_guard<std::recursive_mutex> lock(m_mtx);
		while (this->m_events.try_dequeue(e)) {}
	}

	void invoke(const std::function<void(T)>& event) {
		if (this->m_adaptors.size() < 1) {
			return;
		}

		std::lock_guard<std::recursive_mutex> lock(m_mtx);
		for (auto adaptor : this->m_adaptors) {
			if (adaptor) {
				event(static_cast<T>(adaptor));
			}
		}
	}

	void defer(std::function<void(T)> event) {
		ZstEvent<T> e(event, [](ZstEventStatus s) {});
		this->m_events.enqueue(e);
		m_has_event = true;
		if (auto shared = m_condition_wake.lock())
			shared->notify();
	}

	void defer(std::function<void(T)> event, std::function<void(ZstEventStatus)> on_complete) {
		ZstEvent<T> e(event, on_complete);
		this->m_events.enqueue(e);
		m_has_event = true;
		if (auto shared = m_condition_wake.lock())
			shared->notify();
	}

	void process_events() {
		ZstEvent<T> event;

		while (this->m_events.try_dequeue(event)) {
			bool success = true;

			std::lock_guard<std::recursive_mutex> lock(m_mtx);
			for (auto adaptor : m_adaptors) {
				event.func(static_cast<T>(adaptor));
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
private:
	moodycamel::ConcurrentQueue< ZstEvent<T> > m_events;
};
