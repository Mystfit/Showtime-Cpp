#pragma once

#include <set>
#include <functional>
#include <concurrentqueue.h>


template<typename T>
class ZstEventDispatcher {
public:
	void add_adaptor(T adaptor) { m_adaptors.insert(adaptor); }
	void remove_adaptor(T adaptor) { m_adaptors.erase(adaptor); }
	
	void flush() {
		m_events.consume_all([](){});
	}

	void run_event(const std::function<void(T)> & event) {
		for (T adaptor : m_adaptors) {
			event(adaptor);
		}
	}

	void add_event(std::function<void(T)> event) {
		m_events.enqueue(event);
	}

	void process_events() {
		std::function<void(T)> event_func;

		bool has_event = m_events.try_dequeue(&event_func);
		while (has_event) {
			for (T adaptor : m_adaptors) {
				event_func(adaptor);
			}
			has_event = m_events.try_dequeue(&event_func);
		}
	}

protected:
	std::set<T> adaptors() { return m_adaptors; }

private:
	std::set<T> m_adaptors;
	moodycamel::ConcurrentQueue<std::function<void(T)> > m_events;
};
