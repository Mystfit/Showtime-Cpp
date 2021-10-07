#pragma once

#include <unordered_set>
#include <type_traits>
#include <tuple>
#include <unordered_map>
#include <functional>
#include <concurrentqueue.h>
#include <blockingconcurrentqueue.h>
//#include <readerwriterqueue.h>
#include <mutex>
#include <memory>

#include <showtime/ZstLogging.h>
#include <showtime/ZstPointerUtils.h>
#include <showtime/ZstConstants.h>
#include <showtime/ZstEventDispatcher.h>
#include <showtime/adaptors/ZstEventAdaptor.hpp>
#include "ZstSemaphore.h"

#define EVENT_QUEUE_BLOCK 100

namespace showtime {

enum ZstEventStatus {
	FAILED = 0,
	SUCCESS
};


typedef std::function<void(ZstEventStatus)> ZstEventCallback;


template<typename T>
class ZstEvent 
{
    //static_assert(std::is_base_of<ZstEventAdaptor, T>::value, "T must derive from ZstEventAdaptor");
	//static_assert(std::is_base_of<ZstEventAdaptor, std::remove_pointer_t< std::weak_ptr<T> > >::value, "T must derive from ZstEventAdaptor");
public:
	ZstEvent() : 
		func([](T* adp) {}),
		completed_func([](ZstEventStatus s) {})
	{
	}

	ZstEvent(std::function<void(T*)> f, ZstEventCallback cf) : 
		func(f), 
		completed_func(cf) 
	{
	}
	
	std::function<void(T*)> func;
	ZstEventCallback completed_func;
};


template<typename T>
class ZstEventDispatcherTyped : 
	public IEventDispatcher,
	public inheritable_enable_shared_from_this< ZstEventDispatcherTyped<T> > {
	//static_assert(std::is_base_of< ZstEventAdaptor<T>, T>::value, "T must derive from ZstEventAdaptor");
public:
	ZstEventDispatcherTyped() : 
		m_default_adaptor(std::make_shared<T>()), 
		m_has_event(false)
	{
		// Add default adaptor directly and skip add_daptor since this object will own the adaptor directly
		// -- also avoids having to construct bad weak pointers in the constructor
		this->m_adaptors.insert(m_default_adaptor);
	}

	~ZstEventDispatcherTyped() noexcept {
		/*auto adaptors = m_adaptors;
		for (auto adaptor : adaptors) {
			if (auto adp = adaptor.lock())
				adp->prune_dispatchers();
		}*/
	}

	void add_adaptor(std::weak_ptr<T> adaptor) {
		std::lock_guard<std::recursive_timed_mutex> lock(m_mtx);
		this->m_adaptors.insert(adaptor);
		/*if (auto adp = adaptor.lock())
			adp->add_event_source(ZstEventDispatcherTyped<T>::template downcasted_shared_from_this<ZstEventDispatcherTyped<T>>());*/
	}

	void add_adaptor(T* adaptor) {
		std::lock_guard<std::recursive_timed_mutex> lock(m_mtx);
		this->m_unmanaged_adaptors.insert(adaptor);
		/*if (auto adp = adaptor.lock())
			adp->add_event_source(ZstEventDispatcherTyped<T>::template downcasted_shared_from_this<ZstEventDispatcherTyped<T>>());*/
	}

	void remove_adaptor(std::weak_ptr<T> adaptor) {
		std::lock_guard<std::recursive_timed_mutex> lock(m_mtx);
		/*if (auto adp = adaptor.lock()) {
			adp->remove_event_source(ZstEventDispatcherTyped<T>::template downcasted_shared_from_this<ZstEventDispatcherTyped<T>>());
		}*/
		this->m_adaptors.erase(adaptor);
	}

	void remove_adaptor(T* adaptor) {
		std::lock_guard<std::recursive_timed_mutex> lock(m_mtx);
		/*if (auto adp = adaptor.lock()) {
			adp->remove_event_source(ZstEventDispatcherTyped<T>::template downcasted_shared_from_this<ZstEventDispatcherTyped<T>>());
		}*/
		this->m_unmanaged_adaptors.erase(adaptor);
	}

	bool contains_adaptor(std::weak_ptr<T> adaptor)
	{
		std::lock_guard<std::recursive_timed_mutex> lock(m_mtx);
		return (this->m_adaptors.find(adaptor) != this->m_adaptors.end()) ? true : false;
	}

	bool contains_adaptor(T* adaptor)
	{
		std::lock_guard<std::recursive_timed_mutex> lock(m_mtx);
		return (this->m_unmanaged_adaptors.find(adaptor) != this->m_unmanaged_adaptors.end()) ? true : false;
	}

	size_t num_adaptors() {
		return m_adaptors.size() + m_unmanaged_adaptors.size();
	}

	void remove_all_adaptors() {
		std::lock_guard<std::recursive_timed_mutex> lock(m_mtx);
		
		auto adaptors = m_adaptors;
		for (auto adaptor : adaptors) {
			remove_adaptor(adaptor);
		}
		m_adaptors.clear();

		auto unmanaged_adaptors = m_unmanaged_adaptors;
		for (auto adaptor : unmanaged_adaptors) {
			remove_adaptor(adaptor);
		}
		m_unmanaged_adaptors.clear();
	}

	void prune_missing_adaptors() override {
		std::lock_guard<std::recursive_timed_mutex> lock(m_mtx);
		auto adaptors = m_adaptors;
		for (auto adaptor : adaptors) {
			if (adaptor.expired())
				m_adaptors.erase(adaptor);
		}
	}

	void set_wake_condition(std::shared_ptr<std::condition_variable> condition) {
		std::lock_guard<std::recursive_timed_mutex> lock(m_mtx);
		this->m_condition_wake = condition;
	}

	bool has_event() {
		return m_has_event;
	}

	void notify() {
		if (m_condition_wake)
			m_condition_wake->notify_all();
	}
	
	std::shared_ptr<T> get_default_adaptor() {
		return m_default_adaptor;
	}

	void invoke(std::function<void(T*)> event) {
		if (this->m_adaptors.size() < 1) {
			return;
		}

		//std::lock_guard<std::recursive_timed_mutex> lock(m_mtx);
		for (auto adaptor : this->m_adaptors) {
			if (auto adp = adaptor.lock()) {
				event(adp.get());
			}
		}

		// Unmanaged adaptor invoking
		if (this->m_unmanaged_adaptors.size() < 1) {
			return;
		}
		for (auto adaptor : this->m_unmanaged_adaptors) {
			event(adaptor);
		}
	}

	virtual void process_events() = 0;
	virtual void flush_events() = 0;
	virtual void defer(std::function<void(T*)> event) = 0;
	virtual void defer(std::function<void(T*)> event, ZstEventCallback on_complete) = 0;

protected:
	std::set< std::weak_ptr<T>, std::owner_less< std::weak_ptr<T> > > m_adaptors;
	std::set< T* > m_unmanaged_adaptors;

	std::shared_ptr<std::condition_variable> m_condition_wake;
	std::recursive_timed_mutex m_mtx;
	bool m_has_event;
	
	void process(ZstEvent<T>& event) {
		bool success = true;
		if (m_adaptors.size() > 0) {
			for (auto adaptor : m_adaptors) {
				if (auto adp = adaptor.lock()) {
					try {
						event.func(adp.get());
					}
					catch (std::exception e) {
						Log::net(Log::Level::error, "Event dispatcher failed to run an event on a adaptor. Reason: {}", e.what());
						success = false;
					}
				}
			}
		}

		if (m_unmanaged_adaptors.size() > 0) {
			for (auto adaptor : m_unmanaged_adaptors) {
				try {
					event.func(adaptor);
				}
				catch (std::exception e) {
					Log::net(Log::Level::error, "Event dispatcher failed to run an event on a adaptor. Reason: {}", e.what());
					success = false;
				}
			}
		}

		if(event.completed_func)
			event.completed_func((success) ? ZstEventStatus::SUCCESS : ZstEventStatus::FAILED);
	}

private:
	std::shared_ptr<T> m_default_adaptor;
};


template<typename T>
class ZstEventDispatcher : public ZstEventDispatcherTyped<T>
{
public:
	ZstEventDispatcher() :
		m_events()
	{
	}

	virtual void defer(std::function<void(T*)> event) override {
		ZstEvent<T> e(event, nullptr);
		this->m_events.enqueue(e);
		this->m_has_event = true;
		this->notify();
	}

	virtual void defer(std::function<void(T*)> event, ZstEventCallback on_complete) override {
		ZstEvent<T> e(event, on_complete);
		this->m_events.enqueue(e);
		this->m_has_event = true;
		this->notify();
	}

	virtual void process_events() override {
		ZstEvent<T> event;
		while (this->m_events.try_dequeue(event)) {
			this->process(event);
		}
		this->m_has_event = false;
	}

	virtual void flush_events() override {
		ZstEvent<T> event;
		while (this->m_events.try_dequeue(event)) {}
		this->m_has_event = false;
	}

private:
	moodycamel::ConcurrentQueue< ZstEvent<T> > m_events;
};


template<typename T>
class ZstBlockingEventDispatcher : public ZstEventDispatcherTyped<T>
{
public:
	ZstBlockingEventDispatcher() :
		m_events(EVENT_QUEUE_BLOCK)
	{
	}

	virtual void process_events() override {
		ZstEvent<T> event;
		if (this->m_events.wait_dequeue_timed(event, std::chrono::milliseconds(500)))
			this->process(event);
	}

	virtual void defer(std::function<void(T*)> event) override {
		ZstEvent<T> e(event, [](ZstEventStatus s) {});
		this->m_events.enqueue(e);
		this->m_has_event = true;
	}

	virtual void defer(std::function<void(T*)> event, ZstEventCallback on_complete) override {
		ZstEvent<T> e(event, on_complete);
		this->m_events.enqueue(e);
		this->m_has_event = true;
	}

	virtual void flush_events() override {
		ZstEvent<T> event;
		while (this->m_events.try_dequeue(event)) {}
		this->m_has_event = false;
	}

private:
	moodycamel::BlockingConcurrentQueue< ZstEvent<T> > m_events;
};

}
