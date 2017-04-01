#include "ZstSection.h"
#include "ZstInstrument.h"

using namespace Showtime;

ZstSection::ZstSection(string name)
{
	m_name = name;
}


ZstSection::~ZstSection()
{
}

ZstSection* Showtime::ZstSection::create_section(string name)
{
	return new ZstSection(name);
}

ZstInstrument* ZstSection::create_instrument(string name)
{
	ZstInstrument* instrument = new ZstInstrument(name);
	m_instruments.push_back(instrument);

	return instrument;
}

void ZstSection::destroy_instrument(ZstInstrument& instrument)
{
}

vector<ZstInstrument*>& Showtime::ZstSection::get_instruments()
{
	return m_instruments;
}
