#include "ZstPlugEvent.h"

//ZstPlugEvent
//-------------
ZstPlugEvent::ZstPlugEvent(ZstURI uri, ZstValue & value) : ZstEvent(uri, ZstEvent::EventType::PLUG_HIT)
{
	m_value = new ZstValue(value);
}

ZstPlugEvent::~ZstPlugEvent()
{
	m_value->clear();
	delete m_value;
}

ZstValue * ZstPlugEvent::value()
{
	return m_value;
}
