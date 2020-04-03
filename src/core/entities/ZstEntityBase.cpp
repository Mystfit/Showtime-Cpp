#include <memory>
#include <boost/bimap.hpp>
#include <boost/assign/list_of.hpp>

#include "entities/ZstEntityBase.h"
#include "entities/ZstEntityFactory.h"
#include "ZstCable.h"
#include "../ZstEventDispatcher.hpp"
#include "../ZstHierarchy.h"

using namespace flatbuffers;

namespace showtime
{
    typedef boost::bimap<ZstEntityType, EntityTypes> FlatbuffersEnityTypeMap;
    static const FlatbuffersEnityTypeMap entity_type_lookup = boost::assign::list_of< FlatbuffersEnityTypeMap::relation >
        (ZstEntityType::NONE, EntityTypes_NONE)
        (ZstEntityType::COMPONENT, EntityTypes_Component)
        (ZstEntityType::PERFORMER, EntityTypes_Performer)
        (ZstEntityType::PLUG, EntityTypes_Plug)
        (ZstEntityType::FACTORY, EntityTypes_Factory);


	ZstEntityBase::ZstEntityBase() :
		m_session_events(std::make_shared< ZstEventDispatcher< std::shared_ptr<ZstSessionAdaptor> > >()),
		m_hierarchy_events(std::make_shared< ZstEventDispatcher< std::shared_ptr<ZstHierarchyAdaptor> > >()),
		m_entity_events(std::make_shared< ZstEventDispatcher< std::shared_ptr<ZstEntityAdaptor > > >()),
		m_parent(),
		m_entity_type(ZstEntityType::NONE),
		m_uri(""),
		m_current_owner(""),
		m_registered(false)
    {
    }
    
    ZstEntityBase::ZstEntityBase(const char * name) :
        m_session_events(std::make_shared< ZstEventDispatcher< std::shared_ptr<ZstSessionAdaptor> > >()),
		m_hierarchy_events(std::make_shared< ZstEventDispatcher< std::shared_ptr<ZstHierarchyAdaptor> > >()),
        m_entity_events(std::make_shared< ZstEventDispatcher< std::shared_ptr<ZstEntityAdaptor > > >()),
        m_parent(),
        m_entity_type(ZstEntityType::NONE),
        m_uri(name),
        m_current_owner(""),
		m_registered(false)
    {
    }
    
    ZstEntityBase::ZstEntityBase(const EntityData* buffer) :
        m_session_events(std::make_shared< ZstEventDispatcher< std::shared_ptr<ZstSessionAdaptor> > >()),
		m_hierarchy_events(std::make_shared< ZstEventDispatcher< std::shared_ptr<ZstHierarchyAdaptor> > >()),
        m_entity_events(std::make_shared< ZstEventDispatcher< std::shared_ptr<ZstEntityAdaptor > > >()),
        m_parent(),
        m_entity_type(ZstEntityType::NONE),
        m_uri(""),
        m_current_owner(""),
		m_registered(false)
    {
        deserialize_partial(buffer);
    }

    ZstEntityBase::ZstEntityBase(const ZstEntityBase & other) :
        m_session_events(std::make_shared< ZstEventDispatcher< std::shared_ptr<ZstSessionAdaptor> > >()),
		m_hierarchy_events(std::make_shared< ZstEventDispatcher< std::shared_ptr<ZstHierarchyAdaptor> > >()),
        m_entity_events(std::make_shared< ZstEventDispatcher< std::shared_ptr<ZstEntityAdaptor> > >()),
        m_parent(other.m_parent),
        m_entity_type(other.m_entity_type),
        m_uri(ZstURI(other.m_uri)),
        m_current_owner(other.m_current_owner),
		m_registered(other.m_registered)
    {
    }

    ZstEntityBase::~ZstEntityBase()
    {
        //Let others know this entity is going away
        dispatch_destroyed();
        m_entity_events->remove_all_adaptors();
        m_session_events->remove_all_adaptors();
    }

    ZstEntityBase * ZstEntityBase::parent() const
    {
		ZstEntityBase* parent = NULL;

		if (!is_registered()) {
			ZstLog::entity(LogLevel::warn, "Entity {} not registered. Can't look up parent entity.", URI().path());
		}

		m_hierarchy_events->invoke([&parent, this](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			parent = adaptor->find_entity(m_parent);
		});
        return parent;
    }

	const ZstURI& ZstEntityBase::parent_address() const
	{
		return m_parent;
	}

    void ZstEntityBase::add_child(ZstEntityBase * child, bool auto_activate)
    {
        if(!child)
            return;
        
        if (is_destroyed()) 
			return;

        child->set_parent(this);
    }

    void ZstEntityBase::remove_child(ZstEntityBase * child)
    {
        if(!child)
            return;
        
        std::lock_guard<std::mutex> lock(m_entity_lock);
        child->m_parent = ZstURI();
    }

    void ZstEntityBase::update_URI()
    {		
		std::lock_guard<std::mutex> lock(m_entity_mtx);

        if (m_parent.is_empty()) {
            m_uri = m_uri.last();
            return;
        }
        
        if (!URI().contains(m_parent)) {
            m_uri = m_parent + m_uri.last();
        }
    }

    const ZstEntityType ZstEntityBase::entity_type() const
    {
        return m_entity_type;
    }

    EntityTypes ZstEntityBase::serialized_entity_type()
    {
        try {return entity_type_lookup.left.at(entity_type());}
        catch (std::out_of_range) {}
        return EntityTypes_NONE;
    }

    const ZstURI & ZstEntityBase::URI() const
    {
        return m_uri;
    }

    void ZstEntityBase::get_child_cables(ZstCableBundle & bundle)
    {
    }

    void ZstEntityBase::get_child_entities(ZstEntityBundle & bundle, bool include_parent, bool recursive)
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
    
    void ZstEntityBase::serialize_partial(flatbuffers::Offset<EntityData>& serialized_offset, FlatBufferBuilder& buffer_builder) const
    {
		std::lock_guard<std::mutex> lock(m_entity_mtx);
        auto URI_offset = buffer_builder.CreateString(URI().path(), URI().full_size());
        auto owner_offset = buffer_builder.CreateString(get_owner().path(), get_owner().size());
        serialized_offset = CreateEntityData(buffer_builder, URI_offset, owner_offset);
    }

	uoffset_t ZstEntityBase::serialize(flatbuffers::FlatBufferBuilder & buffer_builder) const
    {
        auto entity_offset = Offset<EntityData>();
        serialize_partial(entity_offset, buffer_builder);
		return CreateEntity(buffer_builder, entity_offset).o;
    }

    void ZstEntityBase::deserialize_partial(const EntityData* buffer)
    {
        if (!buffer) return;

        m_uri = ZstURI(buffer->URI()->c_str(), buffer->URI()->size());
        if(buffer->owner())
            m_current_owner = ZstURI(buffer->owner()->c_str(), buffer->owner()->size());
    }
    
    void ZstEntityBase::deserialize(const Entity* buffer)
    {
		throw(std::runtime_error("Can't deserialize a ZstEntityBase: Class is abstract"));
    }

    void ZstEntityBase::add_adaptor(std::shared_ptr<ZstEntityAdaptor> adaptor)
    {
        this->m_entity_events->add_adaptor(adaptor);
    }

    void ZstEntityBase::add_adaptor(std::shared_ptr<ZstSessionAdaptor> adaptor)
    {
        this->m_session_events->add_adaptor(adaptor);
    }

	void ZstEntityBase::add_adaptor(std::shared_ptr<ZstHierarchyAdaptor> adaptor)
	{
		this->m_hierarchy_events->add_adaptor(adaptor);
	}

    void ZstEntityBase::remove_adaptor(std::shared_ptr<ZstEntityAdaptor> adaptor)
    {
        this->m_entity_events->remove_adaptor(adaptor);
    }

    void ZstEntityBase::remove_adaptor(std::shared_ptr<ZstSessionAdaptor> adaptor)
    {
        this->m_session_events->remove_adaptor(adaptor);
    }

	void ZstEntityBase::remove_adaptor(std::shared_ptr<ZstHierarchyAdaptor> adaptor)
	{
		this->m_hierarchy_events->remove_adaptor(adaptor);
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
		if (!is_registered()) {
			ZstLog::entity(LogLevel::warn, "Entity {} not registered. Can't aquire ownership.", URI().path());
		}

        m_session_events->invoke([this](std::shared_ptr<ZstSessionAdaptor> adaptor) {
			adaptor->aquire_entity_ownership(this);
        });
    }

    void ZstEntityBase::release_ownership()
    {
		if (!is_registered()) {
			ZstLog::entity(LogLevel::warn, "Entity {} not registered. Can't release ownership.", URI().path());
			return;
		}

        m_session_events->invoke([this](std::shared_ptr<ZstSessionAdaptor> adaptor) {
			adaptor->release_entity_ownership(this);
        });
    }

    void ZstEntityBase::on_registered()
    {
    }

    bool ZstEntityBase::is_registered() const
	{
		return m_registered;
	}

	void ZstEntityBase::activate()
	{
		if (!is_registered()) {
			ZstLog::entity(LogLevel::warn, "Entity {} not registered. Can't activate.", URI().path());
			return;
		}

		m_hierarchy_events->invoke([this](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->activate_entity(this, ZstTransportRequestBehaviour::SYNC_REPLY);
		});
	}

	void ZstEntityBase::activate_async()
	{
		if (!is_registered()) {
			ZstLog::entity(LogLevel::warn, "Entity {} not registered. Can't deactivate.", URI().path());
			return;
		}
		
		m_hierarchy_events->invoke([this](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->activate_entity(this, ZstTransportRequestBehaviour::ASYNC_REPLY);
		});
	}

	void ZstEntityBase::deactivate()
	{
		if (!is_registered()) {
			ZstLog::entity(LogLevel::warn, "Entity {} not registered. Can't deactivate.", URI().path());
			return;
		}

		m_hierarchy_events->invoke([this](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->deactivate_entity(this, ZstTransportRequestBehaviour::SYNC_REPLY);
		});
	}

	void ZstEntityBase::deactivate_async()
	{
		if (!is_registered()) {
			ZstLog::entity(LogLevel::warn, "Entity {} not registered. Can't asynchronously deactivate.", URI().path());
			return;
		}

		m_hierarchy_events->invoke([this](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->deactivate_entity(this, ZstTransportRequestBehaviour::ASYNC_REPLY);
		});
	}

    void ZstEntityBase::set_entity_type(ZstEntityType entity_type) {
        std::lock_guard<std::mutex> lock(m_entity_lock);
        m_entity_type = entity_type;
    }

    void ZstEntityBase::set_parent(ZstEntityBase *entity) {
        std::lock_guard<std::mutex> lock(m_entity_lock);

		auto orig_path = this->URI();

        if (entity)
            m_parent = entity->URI();
        else
            m_parent = ZstURI();

        this->update_URI();

		// Update path in entity lookup
		m_hierarchy_events->invoke([this, &orig_path](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->update_entity_URI(this, orig_path);
		});
    }

    void ZstEntityBase::dispatch_destroyed()
    {
		if (activation_status() != ZstSyncStatus::DESTROYED &&
			activation_status() != ZstSyncStatus::DEACTIVATION_QUEUED) {

			// Set child entities and this entity as destroyed so they won't queue destruction events later
			ZstEntityBundle bundle;
			get_child_entities(bundle);
			for (auto child : bundle) {
				child->set_activation_status(ZstSyncStatus::DESTROYED);
			}

            synchronisable_events()->invoke([this](std::weak_ptr<ZstSynchronisableAdaptor> adaptor) {
                if(auto adp = adaptor.lock())
                    adp->on_synchronisable_destroyed(this, true);
            });
        }
    }

    std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstSessionAdaptor> > > & ZstEntityBase::session_events()
    {
        return m_session_events;
    }

	std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstHierarchyAdaptor>>>& ZstEntityBase::hierarchy_events()
	{
		return m_hierarchy_events;
	}

	void ZstEntityBase::set_registered(bool registered)
	{
		std::lock_guard<std::mutex> lock(m_entity_mtx);
        if (m_registered)
            return;

		m_registered = registered;

		if (registered) {
			entity_events()->invoke([this](std::shared_ptr<ZstEntityAdaptor> adaptor) {
				adaptor->on_entity_registered(this);
			});
			this->on_registered();

			//synchronisable_events()->invoke([this](std::shared_ptr<ZstSynchronisableAdaptor> adaptor) {
			//	adaptor->on_synchronisable_has_event(this);
			//});
		}
	}
}
