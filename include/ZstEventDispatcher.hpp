#pragma once

#include <set>
#include <functional>
#include <concurrentqueue.h>

template<typename T>
class ZstEventDispatcher
{
public:
	void add_adaptor(T adaptor) { 
		this->m_adaptors.insert(adaptor); 
	}

	void remove_adaptor(T adaptor) { 
		this->m_adaptors.erase(adaptor); 
	}
	
	void flush() {
		std::function<void(T)> event_func;
		while (this->m_events.try_dequeue(event_func)) {}
	}

	void run_event(const std::function<void(T)> & event) {
		for (T adaptor : this->m_adaptors) {
			event(adaptor);
		}
	}

	void add_event(std::function<void(T)> event) {
		this->m_events.enqueue(event);
	}

	void process_events() {
		std::function<void(T)> event_func;

		while (this->m_events.try_dequeue(event_func)) {
			for (T adaptor : m_adaptors) {
				event_func(adaptor);
			}
		}
	}

private:
	std::set<T> m_adaptors;
	moodycamel::ConcurrentQueue< std::function<void(T)> > m_events;
};
