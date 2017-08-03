#include "ZstEvent.h"
#include "ZstURI.h"

ZstEvent::ZstEvent() :
    m_first(ZstURI()),
    m_second(ZstURI()),
    m_update_type(ZstEvent::NONE)
{
}

ZstEvent::ZstEvent(const ZstEvent & copy) : 
	m_first(copy.m_first),
	m_second(copy.m_second),
	m_update_type(copy.m_update_type)
{
}

ZstEvent::ZstEvent(ZstURI single, EventType event_type) :
	m_first(single),
    m_second(ZstURI()),
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

ZstURI ZstEvent::get_first() const
{
	return m_first;
}

ZstURI ZstEvent::get_second() const
{
	return m_second;
}

ZstEvent::EventType ZstEvent::get_update_type()
{
	return m_update_type;
}

bool ZstEvent::operator==(const ZstEvent & other)
{
	return m_first == other.m_first && m_second == other.m_second;
}

bool ZstEvent::operator!=(const ZstEvent & other)
{
	return !(m_first == other.m_first && m_second == other.m_second);
}
