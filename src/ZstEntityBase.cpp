#include "entities/ZstEntityBase.h"
#include "Showtime.h"
#include "ZstEndpoint.h"
#include <memory>


ZstEntityBase::ZstEntityBase(const char * entity_type, const char * path) :
    m_is_proxy(false),
    m_is_registered(false),
	m_is_destroyed(false),
    m_parent(NULL),
	m_uri(path)
{
	int entity_type_len = strlen(entity_type);
	m_entity_type = (char*)calloc(entity_type_len + 1, sizeof(char));
	strncpy(m_entity_type, entity_type, entity_type_len);
}

ZstEntityBase::ZstEntityBase(const char * entity_type, const char * local_path, ZstEntityBase * parent) :
	m_is_proxy(false),
	m_is_registered(false),
	m_is_destroyed(false),
	m_parent(parent),
	m_uri(parent->URI() + ZstURI(local_path))
{
	int entity_type_len = strlen(entity_type);
	m_entity_type = (char*)calloc(entity_type_len + 1, sizeof(char));
	strncpy(m_entity_type, entity_type, entity_type_len);
	this->parent()->add_child(this);
}

ZstEntityBase::~ZstEntityBase()
{
	free(m_entity_type);
    for(auto child : m_children){
		if (!child.second->is_destroyed()) {
			Showtime::endpoint().destroy_entity(child.second);
		}
    }
    m_children.clear();
    if(m_parent){
        m_parent->remove_child(this);
    }
    m_parent = 0;
	Showtime::endpoint().destroy_entity(this);
}

void ZstEntityBase::activate()
{
    if (Showtime::is_connected() && !m_is_registered && !m_is_proxy){
		m_is_registered = Showtime::endpoint().register_entity(this);
    }
}

const char * ZstEntityBase::entity_type() const
{
	return m_entity_type;
}

const ZstURI & ZstEntityBase::URI()
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

ZstEntityBase * ZstEntityBase::find_child_by_URI(const ZstURI & path) const
{
    ZstEntityBase * result = NULL;
    
    auto entity_iter = m_children.find(path);
    if (entity_iter != m_children.end()) {
        result = entity_iter->second;
    }
    
    return result;
}

ZstEntityBase * ZstEntityBase::get_child_entity_at(int index) const
{
    ZstEntityBase * result;
    int i = 0;
    for(auto it : m_children){
        if(i == index){
            result = it.second;
            break;
        }
        i++;
    }
	return result;
}

const size_t ZstEntityBase::num_children() const
{
	return m_children.size();
}

void ZstEntityBase::add_child(ZstEntityBase * child){
	ZstEntityBase * c = find_child_by_URI(child->URI());
	if (!c) {
		m_children[child->URI()] = child;
	}
}

void ZstEntityBase::remove_child(ZstEntityBase * child){
	auto c = m_children.find(child->URI());
	if (c != m_children.end()) {
		m_children.erase(c);
	}
}
