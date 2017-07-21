#include "ZstEntityBase.h"
#include <memory>

ZstEntityBase::ZstEntityBase(int id, const char * entity_type, const char * entity_name) : m_id(id)
{
	memcpy(m_entity_type, entity_type, 255);
	memcpy(m_name, entity_name, 255);
}

const char * ZstEntityBase::entity_type() const
{
	return m_entity_type;
}

const char * ZstEntityBase::name() const
{
	return m_name;
}

const int ZstEntityBase::id() const
{
	return m_id;
}
