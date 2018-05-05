#include <memory>
#include <msgpack.hpp>

#include <entities/ZstEntityBase.h>
#include <ZstCable.h>

ZstEntityBase::ZstEntityBase(const char * name) : 
	ZstSynchronisable(),
	m_parent(NULL),
	m_entity_type(NULL),
	m_uri(name)
{
}

ZstEntityBase::ZstEntityBase(const ZstEntityBase & other) : ZstSynchronisable(other)
{
	m_parent = other.m_parent;
	
	size_t entity_type_size = strlen(other.m_entity_type);
	m_entity_type = (char*)malloc(entity_type_size + 1);
	memcpy(m_entity_type, other.m_entity_type, entity_type_size);
	m_entity_type[entity_type_size] = '\0';

	m_uri = ZstURI(other.m_uri);
}

ZstEntityBase::~ZstEntityBase()
{
    //destroy();
	set_destroyed();
	free(m_entity_type);
}

void * ZstEntityBase::operator new(size_t num_bytes)
{
	return ::operator new(num_bytes);
}

void ZstEntityBase::operator delete(void * p)
{
	::operator delete(p);
}

ZstEntityBase * ZstEntityBase::parent() const
{
	return m_parent;
}

void ZstEntityBase::update_URI()
{
    if (!parent()) {
        m_uri = m_uri.last();
        return;
    }
    
    bool path_contains_parent = URI().contains(parent()->URI());
    if (!path_contains_parent) {
        m_uri = parent()->URI() + m_uri.last();
    }
}

const char * ZstEntityBase::entity_type() const
{
	return m_entity_type;
}

const ZstURI & ZstEntityBase::URI() const
{
	return m_uri;
}

void ZstEntityBase::release_cable_bundle(ZstCableBundle * bundle)
{
	delete bundle;
}

ZstCableBundle * ZstEntityBase::acquire_cable_bundle()
{
	ZstCableBundle * bundle = new ZstCableBundle();
	return get_child_cables(bundle);
}



ZstCableBundle * ZstEntityBase::get_child_cables(ZstCableBundle * bundle)
{
	return bundle;
}

void ZstEntityBase::write(std::stringstream & buffer) const
{
	msgpack::pack(buffer, URI().path());
	msgpack::pack(buffer, entity_type());
}

void ZstEntityBase::read(const char * buffer, size_t length, size_t & offset)
{
	//Unpack uri path
	auto handle = msgpack::unpack(buffer, length, offset);
	const char * uri = handle.get().via.str.ptr;
	size_t uri_size = handle.get().via.str.size;
	m_uri = ZstURI(uri, uri_size);

	//Unpack entity type second
	handle = msgpack::unpack(buffer, length, offset);
	auto obj = handle.get();

	//Copy entity type string into entity
	m_entity_type = (char*)malloc(obj.via.str.size + 1);
	memcpy(m_entity_type, obj.via.str.ptr, obj.via.str.size);
	m_entity_type[obj.via.str.size] = '\0';
}

void ZstEntityBase::add_adaptor_to_children(ZstSynchronisableAdaptor * adaptor)
{
	ZstSynchronisable::add_adaptor(adaptor);
}

void ZstEntityBase::remove_adaptor_from_children(ZstSynchronisableAdaptor * adaptor)
{
	ZstSynchronisable::remove_adaptor(adaptor);
}

void ZstEntityBase::set_entity_type(const char * entity_type) {
	if (m_entity_type) {
		free(m_entity_type);
		m_entity_type = NULL;
	}
	int entity_type_len = static_cast<int>(strlen(entity_type));
	m_entity_type = (char*)malloc(entity_type_len + 1);
	memcpy(m_entity_type, entity_type, entity_type_len);
    m_entity_type[entity_type_len] = '\0';
}

void ZstEntityBase::set_parent(ZstEntityBase *entity) {
    m_parent = entity;
    this->update_URI();
}
