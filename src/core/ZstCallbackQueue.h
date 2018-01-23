#pragma once

#include <algorithm>
#include <vector>
#include <iostream>
#include "ZstExports.h"
#include "ZstEvents.h"
#include "../core/Queue.h"

template<class Callback, class Target>
class ZstCallbackQueue {
public:
	ZST_EXPORT ZstCallbackQueue() : 
		m_pre_event_callback(NULL), 
		m_post_event_callback(NULL) {
	}

	ZST_EXPORT ~ZstCallbackQueue() {
		if (!m_event_callbacks.size())
			return;
		clear();
	}

	ZST_EXPORT void attach_pre_event_callback(ZstCallbackHook c) {
		m_pre_event_callback.push_back(c);
	}

	ZST_EXPORT void remove_pre_event_callback(ZstCallbackHook c) {
		m_pre_event_callback.erase(std::remove(m_pre_event_callback.begin(), m_pre_event_callback.end(), c), m_pre_event_callback.end());
	}

	ZST_EXPORT void attach_event_listener(Callback * c) {
		m_event_callbacks.push_back(c);
	}

	ZST_EXPORT void remove_event_listener(Callback * c) {
		m_event_callbacks.erase(std::remove(m_event_callbacks.begin(), m_event_callbacks.end(), c), m_event_callbacks.end());
	}

	ZST_EXPORT void attach_post_event_callback(ZstCallbackHook c) {
		m_post_event_callback.push_back(c);
	}

	ZST_EXPORT void remove_post_event_callback(ZstCallbackHook c) {
		m_post_event_callback.erase(std::remove(m_post_event_callback.begin(), m_post_event_callback.end(), c), m_post_event_callback.end());
	}

	ZST_EXPORT void run_event_callbacks(Target t) {
		int i = 0;
		for (int i = 0; i < m_pre_event_callback.size(); ++i) {
			m_pre_event_callback[i](t);
		}

		for (i = 0; i < m_event_callbacks.size(); ++i) {
			m_event_callbacks[i]->run(t);
			m_event_callbacks[i]->increment_calls();
		}

		for (i = 0; i < m_post_event_callback.size(); ++i) {
			m_post_event_callback[i](t);
		}
	}

	ZST_EXPORT void clear() {
		m_event_callbacks.clear();
	}
    
    ZST_EXPORT void enqueue(Target t){
		if (!t) {
			return;
		}
		m_event_queue.push(t);
    }
    
    ZST_EXPORT void process(){
        while(m_event_queue.size() > 0){
			Target t = m_event_queue.pop();
            run_event_callbacks(t);
        }
    }

	ZST_EXPORT size_t size() {
		return m_event_queue.size();
	}

private:
	std::vector<ZstCallbackHook> m_pre_event_callback;
	std::vector<Callback*> m_event_callbacks;
	std::vector<ZstCallbackHook> m_post_event_callback;
    Queue<Target> m_event_queue;
};
