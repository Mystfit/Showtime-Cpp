#include "entities/ZstFilter.h"
#include "ZstEndpoint.h"
#include "Showtime.h"

ZstFilter::ZstFilter(const char * name) : ZstEntityBase(FILTER_TYPE, name)
{
	init();
}

ZstFilter::ZstFilter(const char * name, ZstEntityBase * parent) : ZstEntityBase(FILTER_TYPE, name, parent)
{
	init();
}

void ZstFilter::init()
{
	m_compute_callback = new FilterComputeCallback();
	m_compute_callback->set_target_filter(this);
}

ZstFilter::~ZstFilter()
{
	for (auto plug : m_plugs) {
		Showtime::endpoint().destroy_plug(plug);
	}
	m_plugs.clear();
	delete m_compute_callback;
}

ZstInputPlug * ZstFilter::create_input_plug(const char * name, ZstValueType val_type)
{
	ZstInputPlug * plug = NULL;
	plug = Showtime::endpoint().create_plug<ZstInputPlug>(this, name, val_type, PlugDirection::IN_JACK);
	if (plug) {
		m_plugs.push_back(plug);
		plug->attach_receive_callback(m_compute_callback);
	}
	return plug;
}

ZstOutputPlug * ZstFilter::create_output_plug(const char * name, ZstValueType val_type)
{
	ZstOutputPlug * plug = NULL;
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
	Showtime::endpoint().destroy_plug(plug);
}


// -----------------------
// Filter compute callback
// -----------------------
void FilterComputeCallback::set_target_filter(ZstFilter * filter)
{
	m_filter = filter;
}

void FilterComputeCallback::run(ZstInputPlug * plug)
{
	m_filter->compute(plug);
}
