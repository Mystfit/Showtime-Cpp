#include "entities/ZstComponent.h"
#include "Showtime.h"
#include "ZstPlug.h"
#include "ZstEndpoint.h"
#include "ZstCallbacks.h"


ZstComponent::ZstComponent(const char * entity_type, const char * name) : ZstEntityBase(entity_type, name)
{
	init();
}

ZstComponent::ZstComponent(const char * entity_type, const char * name, ZstEntityBase * parent) : ZstEntityBase(entity_type, name, parent)
{
	init();
}

void ZstComponent::init()
{
	m_compute_callback = new ZstComputeCallback();
	m_compute_callback->set_target_filter(this);
}

ZstComponent::~ZstComponent()
{
	for (auto plug : m_plugs) {
		Showtime::endpoint().destroy_plug(plug);
	}
	m_plugs.clear();
	delete m_compute_callback;
}

ZstInputPlug * ZstComponent::create_input_plug(const char * name, ZstValueType val_type)
{
	ZstInputPlug * plug = NULL;
	plug = Showtime::endpoint().create_plug<ZstInputPlug>(this, name, val_type, PlugDirection::IN_JACK);
	if (plug) {
		m_plugs.push_back(plug);
		plug->attach_receive_callback(m_compute_callback);
	}
	return plug;
}

ZstOutputPlug * ZstComponent::create_output_plug(const char * name, ZstValueType val_type)
{
	ZstOutputPlug * plug = NULL;
	plug = Showtime::endpoint().create_plug<ZstOutputPlug>(this, name, val_type, PlugDirection::OUT_JACK);
	if (plug)
		m_plugs.push_back(plug);
	return plug;
}

ZstPlug * ZstComponent::get_plug_by_URI(const ZstURI uri)
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

void ZstComponent::compute(ZstInputPlug * plug)
{
	std::cout << "Running compute" << std::endl;
}

void ZstComponent::remove_plug(ZstPlug * plug)
{
	m_plugs.erase(std::remove(m_plugs.begin(), m_plugs.end(), plug), m_plugs.end());
	Showtime::endpoint().destroy_plug(plug);
}

