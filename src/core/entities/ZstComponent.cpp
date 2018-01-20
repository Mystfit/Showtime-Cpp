#include "entities/ZstComponent.h"
#include "entities/ZstPlug.h"
#include "ZstCable.h"

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

ZstComponent::~ZstComponent()
{
	for (auto plug : m_plugs) {
		delete plug;
	}
	m_plugs.clear();
	free(m_component_type);
}

void ZstComponent::register_graph_sender(ZstGraphSender * sender)
{
	for (auto plug : m_plugs) {
		plug->register_graph_sender(sender);
	}
}

ZstInputPlug * ZstComponent::create_input_plug(const char * name, ZstValueType val_type)
{
	ZstInputPlug * plug = new ZstInputPlug(name, val_type);
	add_plug(plug);
	return plug;
}

ZstOutputPlug * ZstComponent::create_output_plug(const char * name, ZstValueType val_type)
{
	ZstOutputPlug * plug = new ZstOutputPlug(name, val_type);
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
	for (auto cable : *plug) {
		cable->unplug();
	}
	m_plugs.erase(std::remove(m_plugs.begin(), m_plugs.end(), plug), m_plugs.end());
}

void ZstComponent::write(std::stringstream & buffer)
{
	//Pack container
	ZstEntityBase::write(buffer);

	//Pack component type
	msgpack::pack(buffer, component_type());

	//Pack plugs
	msgpack::pack(buffer, m_plugs.size());
	for (auto plug : m_plugs) {
		plug->write(buffer);
	}
}

void ZstComponent::read(const char * buffer, size_t length, size_t & offset)
{
	//Unpack entity base first
	ZstEntityBase::read(buffer, length, offset);

	//Unpack component type
	auto handle = msgpack::unpack(buffer, length, offset);
	auto obj = handle.get();
	
	set_component_type(handle.get().via.str.ptr, handle.get().via.str.size);

	//Unpack plugs
	handle = msgpack::unpack(buffer, length, offset);
	int num_plugs = static_cast<int>(handle.get().via.i64);
	for (int i = 0; i < num_plugs; ++i) {
		ZstPlug * plug = new ZstPlug();
		plug->read(buffer, length, offset);
		m_plugs.push_back(plug);
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
	strncpy(m_component_type, component_type, len);
	m_component_type[len] = '\0';
}


