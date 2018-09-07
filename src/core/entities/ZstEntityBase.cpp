#include <memory>
#include <msgpack.hpp>

#include <entities/ZstEntityBase.h>
#include <ZstCable.h>
#include <ZstEventDispatcher.hpp>

//Forced template instantiations
template class ZstBundleIterator<ZstCable*>;
template class ZstBundleIterator<ZstEntityBase*>;
template class ZstBundle<ZstCable*>;
template class ZstBundle<ZstEntityBase*>;


ZstEntityBase::ZstEntityBase(const char * name) : 
	ZstSynchronisable(),
	m_parent(NULL),
	m_entity_type(NULL),
	m_uri(name)
{
    m_entity_events = new ZstEventDispatcher<ZstEntityAdaptor*>();
}

ZstEntityBase::ZstEntityBase(const ZstEntityBase & other) : ZstSynchronisable(other)
{
	m_parent = other.m_parent;
	
	size_t entity_type_size = strlen(other.m_entity_type);
	m_entity_type = (char*)malloc(entity_type_size + 1);
	memcpy(m_entity_type, other.m_entity_type, entity_type_size);
	m_entity_type[entity_type_size] = '\0';

	m_uri = ZstURI(other.m_uri);

	m_entity_events = new ZstEventDispatcher<ZstEntityAdaptor*>();
}

ZstEntityBase::~ZstEntityBase()
{
	set_destroyed();
	free(m_entity_type);
    
    m_entity_events->flush();
    m_entity_events->remove_all_adaptors();

    delete m_entity_events;
}

ZstEntityBase * ZstEntityBase::parent() const
{
	return m_parent;
}

void ZstEntityBase::add_child(ZstEntityBase * child)
{
	if (is_destroyed()) return;
	child->set_parent(this);
}

void ZstEntityBase::remove_child(ZstEntityBase * child)
{
	child->m_parent = NULL;
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

ZstCableBundle & ZstEntityBase::get_child_cables(ZstCableBundle & bundle) const
{
	return bundle;
}

ZstEntityBundle & ZstEntityBase::get_child_entities(ZstEntityBundle & bundle, bool include_parent)
{
	if (include_parent) 
		bundle.add(this);
	return bundle;
}

ZstEventDispatcher<ZstEntityAdaptor*> * ZstEntityBase::entity_events()
{
    return m_entity_events;
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

void ZstEntityBase::add_adaptor(ZstEntityAdaptor * adaptor)
{
    this->m_entity_events->add_adaptor(adaptor);
}

void ZstEntityBase::remove_adaptor(ZstEntityAdaptor * adaptor)
{
	this->m_entity_events->remove_adaptor(adaptor);
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
