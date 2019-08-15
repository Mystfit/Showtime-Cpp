#include <memory>
#include <nlohmann/json.hpp>

#include "entities/ZstEntityBase.h"
#include "entities/ZstEntityFactory.h"
#include "ZstCable.h"
#include "../ZstEventDispatcher.hpp"


ZstEntityBase::ZstEntityBase(const char * name) : 
	ZstSynchronisable(),
	m_parent(NULL),
	m_entity_type(""),
	m_uri(name)
{
    m_entity_events = std::make_unique< ZstEventDispatcher<ZstEntityAdaptor*> >();
    m_session_events = std::make_unique< ZstEventDispatcher<ZstSessionAdaptor*> >();
}

ZstEntityBase::ZstEntityBase(const ZstEntityBase & other) : ZstSynchronisable(other)
{
	m_parent = other.m_parent;
	m_entity_type = other.m_entity_type;
	m_uri = ZstURI(other.m_uri);
	m_entity_events = std::make_unique< ZstEventDispatcher<ZstEntityAdaptor*> >();
    m_session_events = std::make_unique< ZstEventDispatcher<ZstSessionAdaptor*> >();
}

ZstEntityBase::~ZstEntityBase()
{
	//Let others know this entity is going away
	dispatch_destroyed();
    
    if(parent()){
        parent()->remove_child(this);
    }

    m_entity_events->remove_all_adaptors();
    m_session_events->remove_all_adaptors();
}

ZstEntityBase * ZstEntityBase::parent() const
{
	return m_parent;
}

void ZstEntityBase::add_child(ZstEntityBase * child, bool auto_activate)
{
    if(!child)
        return;
    
	if (is_destroyed()) return;
	child->set_parent(this);
}

void ZstEntityBase::remove_child(ZstEntityBase * child)
{
    if(!child)
        return;
    
    std::lock_guard<std::mutex> lock(m_entity_lock);
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
	return m_entity_type.c_str();
}

const ZstURI & ZstEntityBase::URI() const
{
	return m_uri;
}

void ZstEntityBase::get_child_cables(ZstCableBundle & bundle)
{
}

void ZstEntityBase::get_child_entities(ZstEntityBundle & bundle, bool include_parent)
{
    if (include_parent){
        std::lock_guard<std::mutex> lock(m_entity_lock);
		bundle.add(this);
    }
}

ZstEventDispatcher<ZstEntityAdaptor*> * ZstEntityBase::entity_events()
{
    return m_entity_events.get();
}

void ZstEntityBase::write_json(json & buffer) const
{
	buffer["URI"] = URI().path();
	buffer["entity_type"] = entity_type();
	buffer["owner"] = get_owner().path();
}

void ZstEntityBase::read_json(const json & buffer)
{
	m_uri = ZstURI(buffer["URI"].get<std::string>().c_str(), buffer["URI"].get<std::string>().size());
	m_entity_type = buffer["entity_type"].get<std::string>();

	if(buffer.find("owner") != buffer.end())
		m_current_owner = ZstURI(buffer["owner"].get<std::string>().c_str(), buffer["owner"].get<std::string>().size());
}

void ZstEntityBase::add_adaptor(ZstEntityAdaptor * adaptor)
{
    this->m_entity_events->add_adaptor(adaptor);
}

void ZstEntityBase::add_adaptor(ZstSessionAdaptor * adaptor)
{
    this->m_session_events->add_adaptor(adaptor);
}

void ZstEntityBase::remove_adaptor(ZstEntityAdaptor * adaptor)
{
	this->m_entity_events->remove_adaptor(adaptor);
}

void ZstEntityBase::remove_adaptor(ZstSessionAdaptor * adaptor)
{
    this->m_session_events->remove_adaptor(adaptor);
}

const ZstURI& ZstEntityBase::get_owner() const
{
    return m_current_owner;
}

void ZstEntityBase::set_owner(const ZstURI& owner)
{
    std::lock_guard<std::mutex> lock(m_entity_lock);
    m_current_owner = owner;
}

void ZstEntityBase::aquire_ownership()
{
    m_session_events->invoke([this](ZstSessionAdaptor* adaptor) {
        adaptor->aquire_entity_ownership(this);
    });
}

void ZstEntityBase::release_ownership()
{
    m_session_events->invoke([this](ZstSessionAdaptor* adaptor) {
        adaptor->release_entity_ownership(this);
    });
}

void ZstEntityBase::set_entity_type(const char * entity_type) {
    std::lock_guard<std::mutex> lock(m_entity_lock);
	m_entity_type = std::string(entity_type);
}

void ZstEntityBase::set_parent(ZstEntityBase *entity) {
    std::lock_guard<std::mutex> lock(m_entity_lock);
	m_parent = entity;
	this->update_URI();
}

void ZstEntityBase::dispatch_destroyed()
{
	if (activation_status() != ZstSyncStatus::DESTROYED) {
		
		//Set child entities and this entity as destroyed so they won't queue destruction events later
		ZstEntityBundle bundle;
		get_child_entities(bundle);
		for (auto c : bundle) {
			c->set_activation_status(ZstSyncStatus::DESTROYED);
		}

		synchronisable_events()->invoke([this](ZstSynchronisableAdaptor * adp) { adp->on_synchronisable_destroyed(this); });
	}
}

ZstEventDispatcher<ZstSessionAdaptor*> * ZstEntityBase::session_events()
{
    return m_session_events.get();
}

