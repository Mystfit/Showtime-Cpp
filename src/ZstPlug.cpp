#include "ZstPlug.h"

using namespace std;

ZstPlug::ZstPlug(string name, string instrument, string performer, Direction direction)
{
	m_direction = direction;
	m_name = name;
    m_instrument = instrument;
	m_performer = performer;
}

ZstPlug::~ZstPlug() {
}

string ZstPlug::get_name()
{
	return m_name;
}

std::string ZstPlug::get_instrument()
{
	return m_instrument;
}

std::string ZstPlug::get_performer()
{
	return m_performer;
}

ZstPlug::Direction ZstPlug::get_direction()
{
	return m_direction;
}

ZstPlug::Address ZstPlug::get_address()
{
	Address address;
	address.performer = m_performer;
	address.instrument = m_instrument;
	address.name = m_name;
	address.direction = m_direction;
	return address;
}
