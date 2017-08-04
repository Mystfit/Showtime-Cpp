#pragma once

#include <algorithm>
#include <vector>
#include "ZstExports.h"

template<class Callback, class Target>
class ZstCallbackQueue {
public:
	ZST_EXPORT ZstCallbackQueue() {
	}

	ZST_EXPORT ~ZstCallbackQueue() {
		if (!m_callbacks.size())
			return;
		clear();
	}

	ZST_EXPORT void attach_event_callback(Callback * c) {
		m_callbacks.push_back(c);
	}

	ZST_EXPORT void remove_event_callback(Callback * c) {
		m_callbacks.erase(std::remove(m_callbacks.begin(), m_callbacks.end(), c), m_callbacks.end());
	}

	ZST_EXPORT void run_event_callbacks(Target t) {
		for (int i = 0; i < m_callbacks.size(); ++i) {
			m_callbacks[i]->run(t);
		}
	}

	ZST_EXPORT void clear() {
		m_callbacks.clear();
	}
private:
	std::vector<Callback*> m_callbacks;
};
