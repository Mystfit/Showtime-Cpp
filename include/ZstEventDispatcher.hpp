#pragma once

#include <set>
#include <functional>
#include <concurrentqueue.h>
#include <ZstLogging.h>

template<typename T>
class ZstEventDispatcher
{
public:
	ZstEventDispatcher(const char * name = "") {
		m_name = std::string(name);
	}

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

	void invoke(const std::function<void(T)> & event) {
		if (this->m_adaptors.size() < 1) {
			ZstLog::net(LogLevel::debug, "Inside dispatcher {} invoke() : No adaptors to pass event to!", m_name, m_events.size_approx());
			return;
		}
		for (T adaptor : this->m_adaptors) {
			event(adaptor);
		}
	}

	void defer(std::function<void(T)> event) {
		this->m_events.enqueue(event);
	}

	void process_events() {
		std::function<void(T)> event_func;

		while (this->m_events.try_dequeue(event_func)) {
			if (m_adaptors.size() < 1) {
				ZstLog::net(LogLevel::debug, "Inside dispatcher {}: No adaptors to pass event to!", m_name);
				continue;
			}
			for (T adaptor : m_adaptors) {
				event_func(adaptor);
			}
		}
	}

private:
	std::set<T> m_adaptors;
	moodycamel::ConcurrentQueue< std::function<void(T)> > m_events;
	std::string m_name;
};
