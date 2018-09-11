#pragma once

#include <set>
#include <functional>
#include <concurrentqueue.h>
#include <ZstLogging.h>

template<typename T>
class ZstEventDispatcher
{
public:
	ZstEventDispatcher(const char * name = "") : m_has_event(false){
		m_name = std::string(name);
	}

	~ZstEventDispatcher() {
		this->flush();
		this->remove_all_adaptors();
	}

	void add_adaptor(T adaptor) { 
		this->m_adaptors.insert(adaptor); 
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
		std::function<void(T)> event_func;
		while (this->m_events.try_dequeue(event_func)) {}
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
		this->m_events.enqueue(event);
		m_has_event = true;
	}

	void process_events() {
		std::function<void(T)> event_func;

		while (this->m_events.try_dequeue(event_func)) {
			for (T adaptor : m_adaptors) {
				event_func(adaptor);
			}
		}
		m_has_event = false;
	}

	bool has_event() {
		return m_has_event;
	}

private:
	std::set<T> m_adaptors;
	moodycamel::ConcurrentQueue< std::function<void(T)> > m_events;
	std::string m_name;
	bool m_has_event;
};
