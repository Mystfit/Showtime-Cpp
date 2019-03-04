#include <memory>
#include <nlohmann/json.hpp>

#include "entities/ZstComponent.h"
#include "../ZstEventDispatcher.hpp"

ZstComponent::ZstComponent() : 
	ZstEntityBase("")
{
	set_entity_type(COMPONENT_TYPE);
	set_component_type("");
}

ZstComponent::ZstComponent(const char * path) :
	ZstEntityBase(path)
{
	set_entity_type(COMPONENT_TYPE);
	set_component_type("");
}

ZstComponent::ZstComponent(const char * component_type, const char * path)
    : ZstEntityBase(path)
{
	set_entity_type(COMPONENT_TYPE);
	set_component_type(component_type);
}

ZstComponent::ZstComponent(const ZstComponent & other) : ZstEntityBase(other)
{
	for (auto p : other.m_plugs) {
		if (p->direction() == ZstPlugDirection::IN_JACK) {
			m_plugs.push_back(new ZstInputPlug(*static_cast<ZstInputPlug*>(p)));
		}
		else if(p->direction() == ZstPlugDirection::OUT_JACK) {
			m_plugs.push_back(new ZstOutputPlug(*static_cast<ZstOutputPlug*>(p)));
		}
	}
	
	m_component_type = other.m_component_type;
}

ZstComponent::~ZstComponent()
{
	if (!is_proxy()) {
		for (auto plug : m_plugs) {
			// TODO: Deleting plugs will crash if the host GC's them after we delete them here
			//ZstLog::entity(LogLevel::debug, "FIXME: Component {} leaking entity {} to avoid host app crashing when GCing", URI().path(), plug->URI().path());
			delete plug;
		}
		m_plugs.clear();
	}
}

ZstInputPlug * ZstComponent::create_input_plug(const char * name, ZstValueType val_type)
{
	return create_input_plug(name, val_type, -1);
}

ZstInputPlug * ZstComponent::create_input_plug(const char * name, ZstValueType val_type, int max_cable_connections)
{
	ZstInputPlug * plug = new ZstInputPlug(name, val_type, max_cable_connections);
	add_plug(plug);
	return plug;
}

ZstOutputPlug * ZstComponent::create_output_plug(const char * name, ZstValueType val_type, bool reliable)
{
	ZstOutputPlug * plug = new ZstOutputPlug(name, val_type, reliable);
	add_plug(plug);
	return plug;
}

ZstPlug * ZstComponent::get_plug_by_URI(const ZstURI & uri) const
{
	ZstPlug * found_plug = NULL;
	for (auto plug : m_plugs) {
		if (ZstURI::equal(uri, plug->URI())) {
			found_plug = plug;
			break;
		}
	}
	return found_plug;
}

int ZstComponent::add_plug(ZstPlug * plug)
{
	int result = 0;
	if (!get_plug_by_URI(plug->URI())) {
		m_plugs.push_back(plug);
		plug->set_parent(this);
		result = 1;
	}
	return result;
}

void ZstComponent::remove_plug(ZstPlug * plug)
{
	if (!plug || !this)
		return;

	ZstCableBundle bundle;
	for (auto cable : plug->get_child_cables(bundle)){
		cable->enqueue_deactivation();
	}
	m_plugs.erase(std::remove(m_plugs.begin(), m_plugs.end(), plug), m_plugs.end());
}

void ZstComponent::set_parent(ZstEntityBase * parent)
{
    ZstEntityBase::set_parent(parent);
    
    std::vector<ZstPlug*> plugs = m_plugs;
    for (auto plug : plugs) {
        plug->set_parent(this);
    }
}

void ZstComponent::write_json(json & buffer) const
{
	//Pack container
	ZstEntityBase::write_json(buffer);

	//Pack component type
	buffer["component_type"] = component_type();

	//Pack plugs
	buffer["plugs"] = json::array();
	for (auto plug : m_plugs) {
		buffer["plugs"].push_back(plug->as_json());
	}
}

void ZstComponent::read_json(const json & buffer)
{
	//Unpack entity base first
	ZstEntityBase::read_json(buffer);

	//Unpack component type
	set_component_type(buffer["component_type"].get<std::string>().c_str(), buffer["component_type"].get<std::string>().size());

	//Unpack plugs
	ZstPlug * plug = NULL;
	ZstPlugDirection direction;

	for (auto p : buffer["plugs"]) {
		//Direction is packed first - we use this to construct the correct plug type
		direction = p["plug_direction"];
		if (direction == ZstPlugDirection::IN_JACK) {
			plug = new ZstInputPlug();
		}
		else {
			plug = new ZstOutputPlug();
		}

		//Unpack plug
		plug->read_json(p);
		add_plug(plug);
	}
}

const char * ZstComponent::component_type() const
{
	return m_component_type.c_str();
}

void ZstComponent::set_component_type(const char * component_type)
{
	set_component_type(component_type, strlen(component_type));
}

void ZstComponent::set_component_type(const char * component_type, size_t len)
{
	m_component_type = std::string(component_type, len);
}

ZstCableBundle & ZstComponent::get_child_cables(ZstCableBundle & bundle) const
{
	for (auto p : m_plugs) {
		p->get_child_cables(bundle);
	}
	return ZstEntityBase::get_child_cables(bundle);
}

ZstEntityBundle & ZstComponent::get_child_entities(ZstEntityBundle & bundle, bool include_parent)
{
	for (auto p : m_plugs) {
		bundle.add(p);
	}
	return ZstEntityBase::get_child_entities(bundle, include_parent);
}
