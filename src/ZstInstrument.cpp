#include "ZstInstrument.h"
#include "ZstPlug.h"

ZstInstrument::~ZstInstrument()
{
}

ZstInstrument * ZstInstrument::create(ZstURI uri)
{
	return new ZstInstrument(uri);
}

void ZstInstrument::compute(ZstPlug * plug)
{
	//Compute some value in here based on the plugs that were run
}

void ZstInstrument::add_plug(ZstPlug * plug)
{
	m_plugs[plug->get_URI()->instrument()].push_back(plug);
}

void ZstInstrument::remove_plug(ZstPlug * plug)
{
	m_plugs[plug->get_URI()->instrument()].erase(
		std::remove(
			m_plugs[plug->get_URI()->instrument()].begin(),
			m_plugs[plug->get_URI()->instrument()].end(), plug
		),
		m_plugs[plug->get_URI()->instrument()].end()
	);
}