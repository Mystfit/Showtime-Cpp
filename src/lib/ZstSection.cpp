#include "ZstSection.h"
#include "ZstInstrument.h"


using namespace Showtime;

ZstSection::ZstSection(string name)
{
	m_name = name;

	string stage_addr = "tcp://127.0.0.1:6000";

	m_toStage = zsock_new_req((">" + stage_addr).c_str());
	m_fromStage = zsock_new_rep("@tcp://127.0.0.1:*");

	m_graph_in = zsock_new_sub(">tcp://127.0.0.1", "");
	m_graph_out = zsock_new_pub("@tcp://127.0.0.1:*", "");

	m_loop = zloop_new();
}


ZstSection::~ZstSection()
{
}

unique_ptr<ZstSection> Showtime::ZstSection::create_section(string name)
{
	return unique_ptr<ZstSection>(new ZstSection(name));
}

shared_ptr<ZstInstrument> ZstSection::create_instrument(string name)
{
	shared_ptr<ZstInstrument> instrument = shared_ptr<ZstInstrument>( new ZstInstrument(name));
	m_instruments.push_back(instrument);

	return instrument;
}

void ZstSection::destroy_instrument(ZstInstrument& instrument)
{
}

vector<shared_ptr<ZstInstrument>>& Showtime::ZstSection::get_instruments()
{
	return m_instruments;
}
