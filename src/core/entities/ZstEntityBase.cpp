#include <memory>

#include "entities/ZstEntityBase.h"
#include "entities/ZstEntityFactory.h"
#include "ZstCable.h"
#include "../ZstEventDispatcher.hpp"

using namespace flatbuffers;

namespace showtime
{
    ZstEntityBase::ZstEntityBase() :
        m_session_events(std::make_shared< ZstEventDispatcher< std::shared_ptr<ZstSessionAdaptor> > >()),
        m_entity_events(std::make_shared< ZstEventDispatcher< std::shared_ptr<ZstEntityAdaptor > > >()),
        m_parent(NULL),
        m_entity_type(EntityTypes_NONE),
        m_uri(""),
        m_current_owner("")
    {
    }
    
    
    ZstEntityBase::ZstEntityBase(const char * name) :
        m_session_events(std::make_shared< ZstEventDispatcher< std::shared_ptr<ZstSessionAdaptor> > >()),
        m_entity_events(std::make_shared< ZstEventDispatcher< std::shared_ptr<ZstEntityAdaptor > > >()),
        m_parent(NULL),
        m_entity_type(EntityTypes_NONE),
        m_uri(name),
        m_current_owner("")
    {
    }
    
    ZstEntityBase::ZstEntityBase(const Entity* buffer) :
        m_session_events(std::make_shared< ZstEventDispatcher< std::shared_ptr<ZstSessionAdaptor> > >()),
        m_entity_events(std::make_shared< ZstEventDispatcher< std::shared_ptr<ZstEntityAdaptor > > >()),
        m_parent(NULL),
        m_entity_type(EntityTypes_NONE),
        m_uri(""),
        m_current_owner("")
    {
    }

    ZstEntityBase::ZstEntityBase(const ZstEntityBase & other) :
        m_session_events(std::make_shared< ZstEventDispatcher< std::shared_ptr<ZstSessionAdaptor> > >()),
        m_entity_events(std::make_shared< ZstEventDispatcher< std::shared_ptr<ZstEntityAdaptor> > >()),
        m_parent(other.m_parent),
        m_entity_type(other.m_entity_type),
        m_uri(ZstURI(other.m_uri)),
        m_current_owner(other.m_current_owner)
    {
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

    const EntityTypes ZstEntityBase::entity_type() const
    {
        return m_entity_type;
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

    std::shared_ptr< ZstEventDispatcher< std::shared_ptr<ZstEntityAdaptor> > > & ZstEntityBase::entity_events()
    {
        return m_entity_events;
    }

    void ZstEntityBase::serialize(flatbuffers::Offset<Entity>& serialized_offset, FlatBufferBuilder& buffer_builder) const
    {
        auto URI_offset = buffer_builder.CreateString(URI().path(), URI().full_size());
        auto owner_offset = buffer_builder.CreateString(get_owner().path(), get_owner().size());
		serialized_offset = CreateEntity(buffer_builder, URI_offset, owner_offset);
    }

    void ZstEntityBase::deserialize_imp(const Entity* buffer)
    {
        m_uri = ZstURI(buffer->URI()->c_str(), buffer->URI()->size());
        m_current_owner = ZstURI(buffer->owner()->c_str(), buffer->owner()->size());
    }
    
    void ZstEntityBase::deserialize(const Entity* buffer)
    {
        ZstEntityBase::deserialize_imp(buffer);
    }

    void ZstEntityBase::add_adaptor(std::shared_ptr<ZstEntityAdaptor> & adaptor)
    {
        this->m_entity_events->add_adaptor(adaptor);
    }

    void ZstEntityBase::add_adaptor(std::shared_ptr<ZstSessionAdaptor> & adaptor)
    {
        this->m_session_events->add_adaptor(adaptor);
    }

    void ZstEntityBase::remove_adaptor(std::shared_ptr<ZstEntityAdaptor> & adaptor)
    {
        this->m_entity_events->remove_adaptor(adaptor);
    }

    void ZstEntityBase::remove_adaptor(std::shared_ptr<ZstSessionAdaptor> & adaptor)
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
        m_session_events->invoke([this](std::weak_ptr<ZstSessionAdaptor> adaptor) {
            if(auto adp = adaptor.lock())
                adp->aquire_entity_ownership(this);
        });
    }

    void ZstEntityBase::release_ownership()
    {
        m_session_events->invoke([this](std::weak_ptr<ZstSessionAdaptor> adaptor) {
            if (auto adp = adaptor.lock())
                adp->release_entity_ownership(this);
        });
    }

    void ZstEntityBase::set_entity_type(EntityTypes entity_type) {
        std::lock_guard<std::mutex> lock(m_entity_lock);
        m_entity_type = entity_type;
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

            synchronisable_events()->invoke([this](std::weak_ptr<ZstSynchronisableAdaptor> adaptor) {
                if(auto adp = adaptor.lock())
                    adp->on_synchronisable_destroyed(this);
            });
        }
    }

    std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstSessionAdaptor> > > & ZstEntityBase::session_events()
    {
        return m_session_events;
    }
}
