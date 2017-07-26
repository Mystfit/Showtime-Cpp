#include "entities\ZstEntityBase.h"
#include "Showtime.h"
#include "ZstEndpoint.h"
#include <memory>

ZstEntityBase::ZstEntityBase() : 
	m_is_registered(false),
	m_parent(NULL)
{
	init();
}

ZstEntityBase::ZstEntityBase(const char * entity_type, const char * entity_name, ZstURI parent) :
	m_is_registered(false),
	m_parent(NULL)
{
	init();
	memcpy(m_entity_type, entity_type, 255);
	memcpy(m_name, entity_name, 255);
}

ZstEntityBase::~ZstEntityBase()
{
	Showtime::destroy_entity(this);
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

const char * ZstEntityBase::name() const
{
	return m_name;
}

ZstURI ZstEntityBase::URI()
{
	return m_uri;
}

bool ZstEntityBase::is_registered()
{
	return m_is_registered;
}

ZstEntityBase * ZstEntityBase::parent() const
{
	return m_parent;
}
