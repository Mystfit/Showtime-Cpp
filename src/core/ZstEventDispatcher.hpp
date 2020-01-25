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
#include "ZstPointerUtils.h"
#include "ZstConstants.h"
#include "adaptors/ZstEventAdaptor.hpp"

#include "ZstSemaphore.h"

namespace showtime {

enum ZstEventStatus {
	FAILED = 0,
	SUCCESS
};


typedef std::function<void(ZstEventStatus)> ZstEventCallback;


template<typename T>
class ZstEvent 
{
    static_assert(std::is_base_of<ZstEventAdaptor, typename std::pointer_traits<T>::element_type >::value, "T must derive from ZstEventAdaptor");
	//static_assert(std::is_base_of<ZstEventAdaptor, std::remove_pointer_t< std::weak_ptr<T> > >::value, "T must derive from ZstEventAdaptor");
public:
	ZstEvent() : func([](T adp) {}), completed_func([](ZstEventStatus) {}) {}
	ZstEvent(std::function<void(T)> f, ZstEventCallback cf) : func(f), completed_func(cf) {};
	
	std::function<void(T)> func;
	ZstEventCallback completed_func;
};


class ZstEventDispatcherBase : public inheritable_enable_shared_from_this<ZstEventDispatcherBase> {
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

		auto adaptors = m_adaptors;
		for (auto adaptor : adaptors) {
			if (auto adp = adaptor.lock())
				adp->prune_dispatchers();
		}
	}

	void add_adaptor(std::weak_ptr<ZstEventAdaptor> adaptor) {
		std::lock_guard<std::recursive_timed_mutex> lock(m_mtx);
		this->m_adaptors.insert(adaptor);
		if(auto adp = adaptor.lock())
			adp->add_event_source(ZstEventDispatcherBase::downcasted_shared_from_this<ZstEventDispatcherBase>());
	}

	void remove_adaptor(std::weak_ptr<ZstEventAdaptor> adaptor) {
		std::lock_guard<std::recursive_timed_mutex> lock(m_mtx);
		if (auto adp = adaptor.lock()) {
			adp->remove_event_source(ZstEventDispatcherBase::downcasted_shared_from_this<ZstEventDispatcherBase>());
		}
		this->m_adaptors.erase(adaptor);
	}

	bool contains_adaptor(std::weak_ptr<ZstEventAdaptor> adaptor)
	{
		std::lock_guard<std::recursive_timed_mutex> lock(m_mtx);
		return (this->m_adaptors.find(adaptor) != this->m_adaptors.end()) ? true : false;
	}

	size_t num_adaptors() {
		return m_adaptors.size();
	}

	void remove_all_adaptors() {
		std::lock_guard<std::recursive_timed_mutex> lock(m_mtx);
		auto adaptors = m_adaptors;
		for (auto adaptor : adaptors) {
			remove_adaptor(adaptor);
		}
		m_adaptors.clear();
	}

	void prune_missing_adaptors() {
		std::lock_guard<std::recursive_timed_mutex> lock(m_mtx);
		auto adaptors = m_adaptors;
		for (auto adaptor : adaptors) {
			if (adaptor.expired())
				m_adaptors.erase(adaptor);
		}
	}

	void set_wake_condition(std::weak_ptr<ZstSemaphore> condition) {
		std::lock_guard<std::recursive_timed_mutex> lock(m_mtx);
		this->m_condition_wake = condition;
	}

	bool has_event() {
		return m_has_event;
	}

protected:
	std::set< std::weak_ptr<ZstEventAdaptor>, std::owner_less< std::weak_ptr<ZstEventAdaptor> > > m_adaptors;
	std::weak_ptr<ZstSemaphore> m_condition_wake;
	std::recursive_timed_mutex m_mtx;
	bool m_has_event;
private:
	std::string m_name;
};


template<typename T>
class ZstEventDispatcher : public ZstEventDispatcherBase
{
    static_assert(std::is_base_of<ZstEventAdaptor, typename std::pointer_traits<T>::element_type >::value, "T must derive from ZstEventAdaptor");
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
		std::lock_guard<std::recursive_timed_mutex> lock(m_mtx);
		while (this->m_events.try_dequeue(e)) {}
	}

	void invoke(const std::function<void(T)>& event) {
		if (this->m_adaptors.size() < 1) {
			return;
		}
		std::lock_guard<std::recursive_timed_mutex> lock(m_mtx);
		for (auto adaptor : this->m_adaptors) {
			if(auto adp = adaptor.lock())
				event(std::static_pointer_cast< typename std::pointer_traits<T>::element_type>(adp));
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

			std::set< std::weak_ptr<ZstEventAdaptor>, std::owner_less< std::weak_ptr<ZstEventAdaptor> > > adaptors;
			auto now = std::chrono::steady_clock::now();
			auto lock = m_mtx.try_lock_until(now + std::chrono::milliseconds(1000));
			if (lock) {
				adaptors = m_adaptors;
			}
			else {
				ZstLog::net(LogLevel::warn, "Could not aquire attached adaptors lock");
				return;
			}
			
			m_mtx.unlock();

			for (auto adaptor : adaptors) {
				if (auto adp = adaptor.lock()) {
					try {
						event.func(std::dynamic_pointer_cast<typename std::pointer_traits<T>::element_type>(adp));
					}
					catch (std::exception e) {
						ZstLog::net(LogLevel::error, "Event dispatcher failed to run an event on a adaptor. Reason: {}", e.what());
						success = false;
					}
				}
			}
			event.completed_func((success) ? ZstEventStatus::SUCCESS : ZstEventStatus::FAILED);
		}
		m_has_event = false;
	}
private:
	moodycamel::ConcurrentQueue< ZstEvent<T> > m_events;
};

}
