#include "ZstInstrument.h" 

using namespace Showtime;

ZstInstrument::ZstInstrument(string name)
{
	m_name = name;
}

ZstInstrument::~ZstInstrument()
{
}

shared_ptr<ZstPlug> Showtime::ZstInstrument::create_plug(string name, ZstPlug::PlugMode plugMode)
{
	shared_ptr<ZstPlug> plug = shared_ptr<ZstPlug>(new ZstPlug(name, plugMode));

	if (plugMode == ZstPlug::PlugMode::READABLE) {
		m_inputs.push_back(plug);
	}
	else if (plugMode == ZstPlug::PlugMode::WRITEABLE) {
		m_outputs.push_back(plug);
	}

	return plug;
}

string ZstInstrument::get_name()
{
	return m_name;
}

vector<shared_ptr<ZstPlug> > Showtime::ZstInstrument::get_outputs()
{
	return m_outputs;
}

vector<shared_ptr<ZstPlug> > Showtime::ZstInstrument::get_inputs()
{
	return m_inputs;
}

