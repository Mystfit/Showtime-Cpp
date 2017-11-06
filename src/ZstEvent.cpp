#include "ZstEvent.h"
#include "ZstURI.h"

ZstEvent::ZstEvent() :
    m_update_type(ZstEvent::NONE)
{
}

ZstEvent::ZstEvent(const ZstEvent & copy) :
	m_update_type(copy.m_update_type),
    m_parameters(copy.m_parameters)
{
}

ZstEvent::ZstEvent(EventType event_type) :
    m_update_type(event_type)
{
}

ZstEvent::~ZstEvent()
{
}

ZstEvent::EventType ZstEvent::get_update_type()
{
	return m_update_type;
}

void ZstEvent::add_parameter(std::string parameter)
{
    m_parameters.push_back(parameter);
}

std::string ZstEvent::get_parameter(size_t index)
{
    return m_parameters[index];
}
