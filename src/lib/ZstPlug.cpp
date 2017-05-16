#include "ZstPlug.h"

using namespace std;

ZstPlug::ZstPlug(string name, string instrument, PlugDirection mode)
{
	m_name = name;
	m_plug_mode = mode;
    m_instrument = instrument;
}

string ZstPlug::get_name()
{
	return m_name;
}

ZstPlug::PlugDirection ZstPlug::get_mode()
{
	return m_plug_mode;
}
