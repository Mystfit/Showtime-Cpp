#pragma once

#include <unordered_set>
#include <type_traits>
#include <tuple>
#include <unordered_map>
#include <functional>
#include <concurrentqueue.h>
#include <mutex>
#include <memory>

#include "ZstLogging.h"
#include "ZstConstants.h"
#include "adaptors/ZstEventAdaptor.hpp"

#include "ZstSemaphore.h"

enum ZstEventStatus {
	FAILED = 0,
	SUCCESS
};


typedef std::function<void(ZstEventStatus)> ZstEventCallback;


template<typename T>
class ZstEvent 
{
	static_assert(std::is_base_of<ZstEventAdaptor, std::pointer_traits<T>::element_type >::value, "T must derive from ZstEventAdaptor");
	//static_assert(std::is_base_of<ZstEventAdaptor, std::remove_pointer_t< std::weak_ptr<T> > >::value, "T must derive from ZstEventAdaptor");
public:
	ZstEvent() : func([](T adp) {}), completed_func([](ZstEventStatus) {}) {}
	ZstEvent(std::function<void(T)> f, ZstEventCallback cf) : func(f), completed_func(cf) {};
	
	std::function<void(T)> func;
	ZstEventCallback completed_func;
};


class ZstEventDispatcherBase : public std::enable_shared_from_this<ZstEventDispatcherBase>{
public:
	ZstEventDispatcherBase() : m_has_event(false)
	{
	}

	ZstEventDispatcherBase(const char* name) : 
		m_has_event(false),
        m_name(name)
	{
	}

	~ZstEventDispatcherBase() noexcept {
		this->remove_all_adaptors();
	}

	void add_adaptor(std::shared_ptr<ZstEventAdaptor> adaptor) {
		//std::lock_guard<std::recursive_mutex> lock(m_mtx);
		this->m_adaptors.insert(adaptor);
		adaptor->add_event_source(shared_from_this());
	}

	void remove_adaptor(std::shared_ptr<ZstEventAdaptor> adaptor) {
		//std::lock_guard<std::recursive_mutex> lock(m_mtx);
		adaptor->set_target_dispatcher_inactive();
		adaptor->remove_event_source(shared_from_this());
		this->m_adaptors.erase(adaptor);
	}

	size_t num_adaptors() {
		return m_adaptors.size();
	}

	void remove_all_adaptors() {
		//std::lock_guard<std::recursive_mutex> lock(m_mtx);
		auto adaptors = m_adaptors;
		for (auto adaptor : adaptors) {
			remove_adaptor(adaptor);
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
	std::unordered_set< std::shared_ptr<ZstEventAdaptor> > m_adaptors;
	std::weak_ptr<ZstSemaphore> m_condition_wake;
	std::recursive_mutex m_mtx;
	bool m_has_event;
private:
	std::string m_name;
};


template<typename T>
class ZstEventDispatcher : public ZstEventDispatcherBase
{
	static_assert(std::is_base_of<ZstEventAdaptor, std::pointer_traits<T>::element_type >::value, "T must derive from ZstEventAdaptor");
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
			event(std::static_pointer_cast<T>(adaptor));
			//event(static_cast<T>(adp));
		}
	}

	void defer(std::function<void(T)> event) {
		ZstEvent<T> e(event, [](ZstEventStatus s) {});
		this->m_events.enqueue(e);
		m_has_event = true;
		if (auto shared = m_condition_wake.lock())
			shared->notify();
	}

	void defer(std::function<void(T)> event, ZstEventCallback on_complete) {
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
				event.func(std::static_pointer_cast<T>(adaptor));
				/*event.func(static_cast<T>(adp));
				try {
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
