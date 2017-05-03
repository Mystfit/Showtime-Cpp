#include "ZstInstrument.h" 

using namespace std;

ZstInstrument::ZstInstrument(string name)
{
	m_name = name;
}

ZstInstrument::~ZstInstrument()
{
}

ZstPlug* ZstInstrument::create_plug(string name, ZstPlug::PlugMode plugMode)
{
	ZstPlug* plug = new ZstPlug(name, plugMode);

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

vector<ZstPlug*> ZstInstrument::get_outputs()
{
	return m_outputs;
}

vector<ZstPlug*> ZstInstrument::get_inputs()
{
	return m_inputs;
}

