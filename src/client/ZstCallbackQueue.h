#pragma once

#include <algorithm>
#include <vector>
#include <iostream>
#include "Queue.h"
#include "ZstExports.h"

//Internal callback hook
typedef void (*ZstCallbackHook)(void*);

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

	ZST_EXPORT void attach_event_callback(Callback * c) {
		m_event_callbacks.push_back(c);
	}

	ZST_EXPORT void remove_event_callback(Callback * c) {
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
			std::cout << "ZST: Can't enqueue event for type NULL" << std::endl;
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

private:
	std::vector<ZstCallbackHook> m_pre_event_callback;
	std::vector<Callback*> m_event_callbacks;
	std::vector<ZstCallbackHook> m_post_event_callback;
    Queue<Target> m_event_queue;
};
