#include "entities\ZstEntityBase.h"
#include <memory>

ZstEntityBase::ZstEntityBase(ZstEntityBehaviour behaviour, const char * entity_type, const char * entity_name) : 
	m_is_registered(false),
	m_id(-1),
	m_behaviour(behaviour),
	m_parent(NULL)
{
	memcpy(m_entity_type, entity_type, 255);
	memcpy(m_name, entity_name, 255);
}

void ZstEntityBase::init()
{
}

ZstEntityBehaviour ZstEntityBase::behaviour() const
{
	return m_behaviour;
}

const char * ZstEntityBase::entity_type() const
{
	return m_entity_type;
}

const char * ZstEntityBase::name() const
{
	return m_name;
}

int ZstEntityBase::id() const
{
	return m_id;
}

bool ZstEntityBase::is_registered()
{
	return m_is_registered;
}

ZstEntityBase * ZstEntityBase::parent() const
{
	return m_parent;
}

const ZstEntityBase * ZstEntityBase::get_child_entity_at(int index) const
{
	return m_children[index];
}

const size_t ZstEntityBase::num_children() const
{
	return m_children.size();
}

ZstPlug * ZstEntityBase::get_plug_by_URI(const ZstURI uri)
{
	ZstPlug * found_plug = NULL;
	//Look for local plugs first
	for (auto plug : m_plugs) {
		if (plug->get_URI() == uri) {
			found_plug = plug;
			break;
		}
	}

	//If no local plug was found, search in children
	if (!found_plug) {
		for (auto child : m_children) {
			found_plug = child->get_plug_by_URI(uri);
			break;
		}
	}
	return found_plug;
}


ZstInputPlug * ZstEntityBase::create_input_plug(const char * name, ZstValueType val_type)
{
	ZstInputPlug * plug = Showtime::create_input_plug(uri, val_type);
	m_plugs.push_back(plug);
}

ZstOutputPlug * ZstEntityBase::create_output_plug(const char * name, ZstValueType val_type)
{
	ZstOutputPlug * plug = Showtime::create_output_plug(uri, val_type);
	m_plugs.push_back(plug);
}

void ZstEntityBase::remove_plug(ZstPlug * plug)
{
	m_plugs.erase(std::remove(m_plugs.begin(),m_plugs.end(), plug), m_plugs.end());
}

void ZstEntityBase::compute(ZstInputPlug * plug)
{
}
