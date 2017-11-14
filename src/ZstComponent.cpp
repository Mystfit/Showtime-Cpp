#include "entities/ZstComponent.h"
#include "Showtime.h"
#include "ZstPlug.h"
#include "ZstEndpoint.h"
#include "ZstCallbacks.h"
#include "ZstPlug.h"


ZstComponent::ZstComponent(const char * entity_type, const char * name)
    : ZstEntityBase(entity_type, name), m_is_registered(false)

{
	init();
}

ZstComponent::~ZstComponent()
{
	for (auto plug : m_plugs) {
		Showtime::endpoint().destroy_plug(plug);
	}
	m_plugs.clear();
}

void ZstComponent::init(){
    activate();
}

void ZstComponent::activate()
{
    if (Showtime::is_connected() && !m_is_registered){
        m_is_registered = Showtime::endpoint().register_entity(this);
    }
}

ZstInputPlug * ZstComponent::create_input_plug(const char * name, ZstValueType val_type)
{
	ZstInputPlug * plug = NULL;
	plug = Showtime::endpoint().create_plug<ZstInputPlug>(this, name, val_type, PlugDirection::IN_JACK);
	if (plug) {
		m_plugs.push_back(plug);
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

ZstPlug * ZstComponent::get_plug_by_URI(const ZstURI & uri) const
{
	ZstPlug * found_plug = NULL;
	for (auto plug : m_plugs) {
		if (ZstURI::equal(uri, plug->get_URI())) {
			found_plug = plug;
			break;
		}
	}
	return found_plug;
}

void ZstComponent::remove_plug(ZstPlug * plug)
{
	m_plugs.erase(std::remove(m_plugs.begin(), m_plugs.end(), plug), m_plugs.end());
	Showtime::endpoint().destroy_plug(plug);
}

