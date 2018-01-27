#include <ZstEventDispatcher.h>

ZstEventDispatcher::ZstEventDispatcher() :
	m_pre_event_callback(NULL),
	m_post_event_callback(NULL) {
}

ZstEventDispatcher::~ZstEventDispatcher() {
	if (!m_event_callbacks.size())
		return;
	clear();
}

void ZstEventDispatcher::attach_pre_event_callback(ZstEvent * event) {
	m_pre_event_callback.push_back(event);
}

void ZstEventDispatcher::remove_pre_event_callback(ZstEvent * event) {
	m_pre_event_callback.erase(std::remove(m_pre_event_callback.begin(), m_pre_event_callback.end(), event), m_pre_event_callback.end());
}

void ZstEventDispatcher::attach_event_listener(ZstEvent * event) {
	m_event_callbacks.push_back(event);
}

void ZstEventDispatcher::remove_event_listener(ZstEvent * event) {
	m_event_callbacks.erase(std::remove(m_event_callbacks.begin(), m_event_callbacks.end(), event), m_event_callbacks.end());
}

void ZstEventDispatcher::attach_post_event_callback(ZstEvent * event) {
	m_post_event_callback.push_back(event);
}

void ZstEventDispatcher::remove_post_event_callback(ZstEvent * event) {
	m_post_event_callback.erase(std::remove(m_post_event_callback.begin(), m_post_event_callback.end(), event), m_post_event_callback.end());
}

void ZstEventDispatcher::dispatch_events(ZstSynchronisable * target) {
	int i = 0;
	for (auto c : m_pre_event_callback){
		c->cast_run(target);
	}

	for (auto c : m_event_callbacks) {
		c->cast_run(target);
	}

	for (auto c : m_post_event_callback) {
		c->cast_run(target);
	}
}

void ZstEventDispatcher::clear() {
	m_event_callbacks.clear();
}

void ZstEventDispatcher::enqueue(ZstSynchronisable * target) {
	if (!target) {
		return;
	}
	m_event_queue.push(target);
}

void ZstEventDispatcher::process() {
	while (m_event_queue.size() > 0) {
		ZstSynchronisable * target = m_event_queue.front();
		m_event_queue.pop();
		dispatch_events(target);
	}
}

size_t ZstEventDispatcher::size() {
	return m_event_queue.size();
}