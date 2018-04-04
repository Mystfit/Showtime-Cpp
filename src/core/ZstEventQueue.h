#pragma once

#include <algorithm>
#include <vector>
#include <queue>
#include <iostream>
#include <boost/lockfree/queue.hpp>
#include <ZstExports.h>
#include <ZstEvents.h>

class ZstEventQueue {
public:
	ZST_EXPORT ZstEventQueue();
	ZST_EXPORT ~ZstEventQueue();
	ZST_EXPORT void attach_pre_event_callback(ZstEvent * event);
	ZST_EXPORT void remove_pre_event_callback(ZstEvent * event);
	ZST_EXPORT void attach_event_listener(ZstEvent * event);
	ZST_EXPORT void remove_event_listener(ZstEvent * event);
	ZST_EXPORT void attach_post_event_callback(ZstEvent * event);
	ZST_EXPORT void remove_post_event_callback(ZstEvent * event);
	ZST_EXPORT void dispatch_events(ZstSynchronisable * target);
	ZST_EXPORT void flush();
    ZST_EXPORT void clear_attached_events();
	ZST_EXPORT void enqueue(ZstSynchronisable * target);
	ZST_EXPORT void process();

private:
	std::vector<ZstEvent*> m_pre_event_callback;
	std::vector<ZstEvent*> m_event_callbacks;
	std::vector<ZstEvent*> m_post_event_callback;
	boost::lockfree::queue<ZstSynchronisable*> m_event_queue;
};
