#include "entities/ZstEntityBase.h"
#include "Showtime.h"
#include "ZstEndpoint.h"
#include <memory>


ZstEntityBase::ZstEntityBase(const char * entity_type, const char * path) :
	m_is_registered(false),
	m_is_destroyed(false),
    m_parent(NULL)
{
	memcpy(m_entity_type, entity_type, 255);
	m_uri = ZstURI(path);
	init();
}

ZstEntityBase::ZstEntityBase(const char * entity_type, const char * local_path, ZstEntityBase * parent) :
	m_is_registered(false),
	m_is_destroyed(false),
    m_parent(parent)
{
	memcpy(m_entity_type, entity_type, 255);
	m_uri = ZstURI::join(parent->URI(), ZstURI(local_path));
	init();
}

ZstEntityBase::~ZstEntityBase()
{
	Showtime::endpoint().destroy_entity(this);
}

void ZstEntityBase::init()
{
	if (Showtime::is_connected())
		m_is_registered = Showtime::endpoint().register_entity(this);
}

const char * ZstEntityBase::entity_type() const
{
	return m_entity_type;
}

ZstURI ZstEntityBase::URI()
{
	return m_uri;
}

bool ZstEntityBase::is_registered()
{
	return m_is_registered;
}

bool ZstEntityBase::is_destroyed()
{
	return m_is_destroyed;
}

void ZstEntityBase::set_destroyed()
{
	m_is_destroyed = true;
}

ZstEntityBase * ZstEntityBase::parent() const
{
	return m_parent;
}

ZstEntityBase * ZstEntityBase::get_child_entity_at(int index) const
{
	return m_children[index];
}

const size_t ZstEntityBase::num_children() const
{
	return m_children.size();
}
