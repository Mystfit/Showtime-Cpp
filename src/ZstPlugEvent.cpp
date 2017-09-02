#include "ZstPlugEvent.h"

//ZstPlugEvent
//-------------
ZstPlugEvent::ZstPlugEvent(ZstURI uri, ZstValue & value) : ZstEvent(uri, ZstEvent::EventType::PLUG_HIT)
{
	m_value = value;
}

ZstPlugEvent::~ZstPlugEvent()
{
	m_value.clear();
}

ZstValue & ZstPlugEvent::value()
{
	return m_value;
}
