#include <memory>
#include <iostream>
#include <cstring>
#include "entities/ZstEntityBase.h"
#include "msgpack.hpp"

ZstEntityBase::ZstEntityBase(const char * entity_type, const char * name) :
    m_uri(name),
	m_is_activated(false),
	m_is_destroyed(false),
	m_parent(NULL)
{
	set_entity_type(entity_type);
}

ZstEntityBase::~ZstEntityBase()
{
    //destroy();
	set_destroyed();
	free(m_entity_type);
}

bool ZstEntityBase::is_activated()
{
	return m_is_activated;
}

ZstEntityBase * ZstEntityBase::parent() const
{
	return m_parent;
}

const char * ZstEntityBase::entity_type() const
{
	return m_entity_type;
}

const ZstURI & ZstEntityBase::URI()
{
	return m_uri;
}

bool ZstEntityBase::is_destroyed()
{
	return m_is_destroyed;
}

void ZstEntityBase::set_destroyed()
{
	m_is_destroyed = true;
}

void ZstEntityBase::set_activated()
{
	m_is_activated = true;
}

void * ZstEntityBase::operator new(size_t num_bytes)
{
	return ::operator new(num_bytes);
}

void ZstEntityBase::operator delete(void * p)
{
	::operator delete(p);
}

void ZstEntityBase::write(std::stringstream & buffer)
{
	msgpack::pack(buffer, URI().path());
	msgpack::pack(buffer, entity_type());
}

void ZstEntityBase::read(const char * buffer, size_t length, size_t & offset)
{
	//Unpack uri path
	auto handle = msgpack::unpack(buffer, length, offset);
	m_uri = ZstURI(handle.get().via.str.ptr, handle.get().via.str.size);

	//Unpack entity type second
	handle = msgpack::unpack(buffer, length, offset);
	auto obj = handle.get();

	//Copy entity type string into entity
	m_entity_type = (char*)malloc(obj.via.str.size + 1);
	strncpy(m_entity_type, obj.via.str.ptr, obj.via.str.size);
	m_entity_type[obj.via.str.size] = '\0';
}

void ZstEntityBase::set_entity_type(const char * entity_type) {
	int entity_type_len = static_cast<int>(strlen(entity_type));
	m_entity_type = (char*)calloc(entity_type_len + 1, sizeof(char));
	strncpy(m_entity_type, entity_type, entity_type_len);
}

void ZstEntityBase::set_parent(ZstEntityBase *entity) {
	if (!entity->URI().contains(URI())) {
		m_uri = entity->URI() + m_uri;
	}
	m_parent = entity;
}
