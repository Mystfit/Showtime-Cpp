#include "entities\ZstFilter.h"
#include "ZstEndpoint.h"

ZstInputPlug * ZstFilter::create_input_plug(const char * name, ZstValueType val_type)
{
	ZstInputPlug * plug = NULL;
	ZstURI plug_uri = ZstURI(URI().instrument_char(), name);
	plug = Showtime::endpoint().create_plug<ZstInputPlug>(this, name, val_type, PlugDirection::IN_JACK);
	if (plug)
		m_plugs.push_back(plug);
	return plug;
}

ZstOutputPlug * ZstFilter::create_output_plug(const char * name, ZstValueType val_type)
{
	ZstOutputPlug * plug = NULL;
	ZstURI plug_uri = ZstURI(URI().instrument_char(), name);
	plug = Showtime::endpoint().create_plug<ZstOutputPlug>(this, name, val_type, PlugDirection::OUT_JACK);
	if(plug)
		m_plugs.push_back(plug);
	return plug;
}

ZstPlug * ZstFilter::get_plug_by_URI(const ZstURI uri)
{
	ZstPlug * found_plug = NULL;
	for (auto plug : m_plugs) {
		if (plug->get_URI() == uri) {
			found_plug = plug;
			break;
		}
	}
	return found_plug;
}

void ZstFilter::remove_plug(ZstPlug * plug)
{
	m_plugs.erase(std::remove(m_plugs.begin(), m_plugs.end(), plug), m_plugs.end());
}