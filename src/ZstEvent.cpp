#include "ZstEvent.h"
#include "ZstURI.h"

ZstEvent::ZstEvent()
{
}

ZstEvent::ZstEvent(const ZstEvent & copy)
{
	m_first = copy.m_first;
	m_second = copy.m_second;
	m_update_type = copy.m_update_type;
}

ZstEvent::ZstEvent(ZstURI single, EventType event_type) :
	m_first(single),
	m_update_type(event_type)
{
}

ZstEvent::ZstEvent(ZstURI first, ZstURI second, EventType event_type) :
	m_first(first),
	m_second(second),
	m_update_type(event_type)
{
}

ZstEvent::~ZstEvent()
{
}

ZstURI ZstEvent::get_first()
{
	return m_first;
}

ZstURI ZstEvent::get_second()
{
	return m_second;
}

ZstEvent::EventType ZstEvent::get_update_type()
{
	return EventType();
}
