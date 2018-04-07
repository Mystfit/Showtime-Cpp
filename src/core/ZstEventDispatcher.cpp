#include "ZstEventDispatcher.h"

ZstEventDispatcher::~ZstEventDispatcher()
{
	m_event_queues.clear();
}

void ZstEventDispatcher::add_event_queue(ZstEventQueue * queue)
{
	m_event_queues.insert(queue);
}

void ZstEventDispatcher::remove_event_queue(ZstEventQueue * queue)
{
	m_event_queues.erase(queue);
}

void ZstEventDispatcher::process_callbacks()
{
	for (ZstEventQueue * queue : m_event_queues) {
		queue->process();
	}
}

void ZstEventDispatcher::flush()
{
	for (ZstEventQueue* queue : m_event_queues) {
		queue->flush();
	}
}
