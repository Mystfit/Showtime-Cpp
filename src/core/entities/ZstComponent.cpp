#include <memory>
#include <msgpack.hpp>
#include <entities/ZstComponent.h>
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
	
	size_t component_type_size = strlen(other.m_component_type);
	m_component_type = (char*)malloc(component_type_size + 1);
	memcpy(m_component_type, other.m_component_type, component_type_size);
	m_component_type[component_type_size] = '\0';
}

ZstComponent::~ZstComponent()
{


	for (auto p : m_plugs) {
		delete p;
	}
	m_plugs.clear();
	free(m_component_type);
}

ZstInputPlug * ZstComponent::create_input_plug(const char * name, ZstValueType val_type)
{
	ZstInputPlug * plug = new ZstInputPlug(name, val_type);
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
	if (!plug)
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

void ZstComponent::write(std::stringstream & buffer) const
{
	//Pack container
	ZstEntityBase::write(buffer);

	//Pack component type
	msgpack::pack(buffer, component_type());

	//Pack plugs
	msgpack::pack(buffer, m_plugs.size());
	for (auto plug : m_plugs) {
		msgpack::pack(buffer, static_cast<int>(plug->direction()));
		plug->write(buffer);
	}
}

void ZstComponent::read(const char * buffer, size_t length, size_t & offset)
{
	//Unpack entity base first
	ZstEntityBase::read(buffer, length, offset);

	//Unpack component type
	auto handle = msgpack::unpack(buffer, length, offset);
	set_component_type(handle.get().via.str.ptr, handle.get().via.str.size);

	//Unpack plugs
	handle = msgpack::unpack(buffer, length, offset);
	int num_plugs = static_cast<int>(handle.get().via.i64);
	ZstPlugDirection direction;
	ZstPlug * plug = NULL;

	for (int i = 0; i < num_plugs; ++i) {
		//Direction is packed first - we use this to construct the correct plug type
		handle = msgpack::unpack(buffer, length, offset);
		direction = static_cast<ZstPlugDirection>(handle.get().via.i64);
		if (direction == ZstPlugDirection::IN_JACK) {
			plug = new ZstInputPlug();
		} else {
			plug = new ZstOutputPlug();
		}

		//Unpack plug
		plug->read(buffer, length, offset);
		add_plug(plug);
	}
}

const char * ZstComponent::component_type() const
{
	return m_component_type;
}

void ZstComponent::set_component_type(const char * component_type)
{
	set_component_type(component_type, strlen(component_type));
}

void ZstComponent::set_component_type(const char * component_type, size_t len)
{
	m_component_type = (char*)malloc(len + 1);
    memcpy(m_component_type, component_type, len);
	m_component_type[len] = '\0';
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
