#include <ZstSynchronisable.h>
#include <ZstLogging.h>
#include "ZstEventDispatcher.h"

ZstEventDispatcher::ZstEventDispatcher() :
	m_pre_event_callback(NULL),
	m_post_event_callback(NULL),
	m_event_queue{50} {
}

ZstEventDispatcher::~ZstEventDispatcher() {
	m_event_callbacks.clear();
	m_pre_event_callback.clear();
	m_post_event_callback.clear();
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

void ZstEventDispatcher::flush() {
	m_event_queue.consume_all([](ZstSynchronisable* target) {});
}

void ZstEventDispatcher::clear_attached_events() {
    m_pre_event_callback.clear();
    m_event_callbacks.clear();
    m_post_event_callback.clear();
}


void ZstEventDispatcher::enqueue(ZstSynchronisable * target) {
	if (!target) {
		return;
	}
	m_event_queue.push(target);
}

void ZstEventDispatcher::process() {
	m_event_queue.consume_all([this](ZstSynchronisable* target) {
		try {
			this->dispatch_events(target);
		}
		catch(std::exception e) {
			ZstLog::entity(LogLevel::error, "Failed to dispatch event. Reason: {}", e.what());
		}
	});
}
