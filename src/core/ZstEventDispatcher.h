#pragma once

#include <algorithm>
#include <vector>
#include <queue>
#include <iostream>
#include <ZstExports.h>
#include <ZstEvents.h>

#include <concurrentqueue.h>

class ZstEventDispatcher {
public:
	ZST_EXPORT ZstEventDispatcher();
	ZST_EXPORT ~ZstEventDispatcher();
	ZST_EXPORT void attach_pre_event_callback(ZstEvent * event);
	ZST_EXPORT void remove_pre_event_callback(ZstEvent * event);
	ZST_EXPORT void attach_event_listener(ZstEvent * event);
	ZST_EXPORT void remove_event_listener(ZstEvent * event);
	ZST_EXPORT void attach_post_event_callback(ZstEvent * event);
	ZST_EXPORT void remove_post_event_callback(ZstEvent * event);
	ZST_EXPORT void dispatch_events(ZstSynchronisable * target);
	ZST_EXPORT void clear();
	ZST_EXPORT void enqueue(ZstSynchronisable * target);
	ZST_EXPORT void process();
	ZST_EXPORT size_t size();

private:
	std::vector<ZstEvent*> m_pre_event_callback;
	std::vector<ZstEvent*> m_event_callbacks;
	std::vector<ZstEvent*> m_post_event_callback;
	moodycamel::ConcurrentQueue<ZstSynchronisable*> m_event_queue;
};
