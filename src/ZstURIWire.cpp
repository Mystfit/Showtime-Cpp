#include "ZstURIWire.h"

using namespace std;

ZstURIWire::ZstURIWire()
{
}

ZstURIWire::ZstURIWire(const ZstURIWire & copy)
{
	int performer_size = strlen(copy.m_performer);
	int instrument_size = strlen(copy.m_instrument);
	int name_size = strlen(copy.m_name);

	m_performer = new char[performer_size + 1]();
	m_instrument = new char[instrument_size + 1]();
	m_name = new char[name_size + 1]();

	strncpy(m_performer, copy.m_performer, performer_size);
	strncpy(m_instrument, copy.m_instrument, instrument_size);
	strncpy(m_name, copy.m_name, name_size);

	m_direction = copy.m_direction;
}

ZstURIWire::ZstURIWire(ZstURI uri)
{
	int performer_size = strlen(uri.performer_char());
	int instrument_size = strlen(uri.performer_char());
	int name_size = strlen(uri.performer_char());

	m_performer = new char[performer_size + 1]();
	m_instrument = new char[instrument_size + 1]();
	m_name = new char[name_size + 1]();

	strncpy(m_performer, uri.performer_char(), performer_size);
	strncpy(m_instrument, uri.instrument_char(), instrument_size);
	strncpy(m_name, uri.name_char(), name_size);
	m_direction = uri.direction();
}
