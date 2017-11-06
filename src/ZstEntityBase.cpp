#include "entities/ZstEntityBase.h"
#include "Showtime.h"
#include "ZstEndpoint.h"
#include <memory>
#include <cstring>


ZstEntityBase::ZstEntityBase() :
    m_entity_type(NULL),
    m_uri(),
    m_is_destroyed(false),
    m_parent(NULL)
{
}

ZstEntityBase::ZstEntityBase(const char * entity_type, const char * path) :
    m_uri(path),
	m_is_destroyed(false),
    m_parent(NULL)
{
	int entity_type_len = strlen(entity_type);
	m_entity_type = (char*)calloc(entity_type_len + 1, sizeof(char));
	strncpy(m_entity_type, entity_type, entity_type_len);
}

ZstEntityBase::ZstEntityBase(const char * entity_type, const char * local_path, ZstEntityBase * parent) :
    m_uri(parent->URI() + ZstURI(local_path)),
    m_is_destroyed(false),
	m_parent(parent)
{
	int entity_type_len = strlen(entity_type);
	m_entity_type = (char*)calloc(entity_type_len + 1, sizeof(char));
	strncpy(m_entity_type, entity_type, entity_type_len);
	this->parent()->add_child(this);
}

ZstEntityBase::~ZstEntityBase()
{
	Showtime::endpoint().destroy_entity(this);
	set_destroyed();

	//Make a copy of the children list so we can remove whilst iterating
	std::vector<ZstEntityBase*> removed_children;
    for(auto child : m_children){
		if (!child.second->is_destroyed()) {
			removed_children.push_back(child.second);
		}
    }
	for (auto child : removed_children) {
		Showtime::endpoint().destroy_entity(child);
	}
	
    m_children.clear();
	removed_children.clear();
    m_parent = NULL;
	free(m_entity_type);
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
