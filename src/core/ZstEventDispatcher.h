#pragma once

#include <set>
#include <ZstExports.h>
#include "ZstEventQueue.h"

class ZstEventDispatcher {
public:
	ZST_EXPORT ZstEventDispatcher() {};
	ZST_EXPORT ~ZstEventDispatcher();

	ZST_EXPORT void add_event_queue(ZstEventQueue * queue);
	ZST_EXPORT void remove_event_queue(ZstEventQueue * queue);
	ZST_EXPORT virtual void process_callbacks();

private:
	std::set<ZstEventQueue*> m_event_queues;
};