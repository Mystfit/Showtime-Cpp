#include "ZstPlugEvent.h"

//ZstPlugEvent
//-------------
ZstPlugEvent::ZstPlugEvent(const ZstURI & uri, const ZstValue & value) : ZstEvent(ZstEvent::EventType::PLUG_HIT)
{
    add_parameter(std::string(uri.path()));
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
