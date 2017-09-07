#include "entities/ZstEntityBase.h"
#include "Showtime.h"
#include "ZstEndpoint.h"
#include <memory>


ZstEntityBase::ZstEntityBase(const char * entity_type, const char * path) :
	m_is_registered(false),
	m_is_destroyed(false),
    m_parent(NULL),
    m_is_proxy(false)
{
	memcpy(m_entity_type, entity_type, 255);
	m_uri = ZstURI(path);
}

ZstEntityBase::ZstEntityBase(const char * entity_type, const char * local_path, ZstEntityBase * parent) :
	m_is_registered(false),
	m_is_destroyed(false),
    m_parent(parent),
    m_is_proxy(false)
{
	memcpy(m_entity_type, entity_type, 255);
	m_uri = ZstURI::join(parent->URI(), ZstURI(local_path));
}

ZstEntityBase::~ZstEntityBase()
{
	Showtime::endpoint().destroy_entity(this);
}
    
void ZstEntityBase::activate()
{
    if (Showtime::is_connected() && !m_is_registered){
		m_is_registered = Showtime::endpoint().register_entity(this);
    }
}

const char * ZstEntityBase::entity_type() const
{
	return m_entity_type;
}

ZstURI & ZstEntityBase::URI()
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

bool ZstEntityBase::is_proxy(){
    return m_is_proxy;
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
