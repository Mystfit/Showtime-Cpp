#include "entities\ZstEntityBase.h"
#include "Showtime.h"
#include "ZstEndpoint.h"
#include <memory>

ZstEntityBase::ZstEntityBase()
{
	init();
}

ZstEntityBase::ZstEntityBase(const char * entity_type, const char * path) :
	m_is_registered(false),
	m_parent(NULL)
{
	memcpy(m_entity_type, entity_type, 255);
	m_uri = ZstURI(path);
	init();
}

ZstEntityBase::ZstEntityBase(const char * entity_type, const char * local_path, ZstEntityBase * parent) :
	m_is_registered(false),
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

ZstEntityBase * ZstEntityBase::parent() const
{
	return m_parent;
}
