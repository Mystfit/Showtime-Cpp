#include <memory>
#include <boost/bimap.hpp>
#include <boost/assign/list_of.hpp>

#include <showtime/entities/ZstEntityBase.h>
#include <showtime/entities/ZstEntityFactory.h>
#include <showtime/ZstCable.h>
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
		m_session_events(std::make_shared< ZstEventDispatcher<ZstSessionAdaptor> >()),
		m_hierarchy_events(std::make_shared< ZstEventDispatcher<ZstHierarchyAdaptor> >()),
		m_entity_events(std::make_shared< ZstEventDispatcher<ZstEntityAdaptor > >()),
		m_parent(),
		m_entity_type(ZstEntityType::NONE),
		m_uri(""),
		m_current_owner(""),
		m_registered(false)
    {
    }
    
    ZstEntityBase::ZstEntityBase(const char * name) :
        m_session_events(std::make_shared< ZstEventDispatcher<ZstSessionAdaptor> >()),
		m_hierarchy_events(std::make_shared< ZstEventDispatcher<ZstHierarchyAdaptor> >()),
        m_entity_events(std::make_shared< ZstEventDispatcher<ZstEntityAdaptor > >()),
        m_parent(),
        m_entity_type(ZstEntityType::NONE),
        m_uri(name),
        m_current_owner(""),
		m_registered(false)
    {
    }
    
    ZstEntityBase::ZstEntityBase(const EntityData* buffer) :
        m_session_events(std::make_shared< ZstEventDispatcher<ZstSessionAdaptor> >()),
		m_hierarchy_events(std::make_shared< ZstEventDispatcher<ZstHierarchyAdaptor> >()),
        m_entity_events(std::make_shared< ZstEventDispatcher<ZstEntityAdaptor > >()),
        m_parent(),
        m_entity_type(ZstEntityType::NONE),
        m_uri(""),
        m_current_owner(""),
		m_registered(false)
    {
        deserialize_partial(buffer);
    }

    ZstEntityBase::ZstEntityBase(const ZstEntityBase & other) :
        m_session_events(std::make_shared< ZstEventDispatcher<ZstSessionAdaptor> >()),
		m_hierarchy_events(std::make_shared< ZstEventDispatcher<ZstHierarchyAdaptor> >()),
        m_entity_events(std::make_shared< ZstEventDispatcher<ZstEntityAdaptor> >()),
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
        if (parent_address().is_empty())
            return parent;

		if (!is_registered()) {
			Log::entity(Log::Level::warn, "Entity {} not registered. Can't look up parent entity.", URI().path());
            return parent;
		}

		m_hierarchy_events->invoke([&parent, this](ZstHierarchyAdaptor* adaptor) {
			auto entity = adaptor->find_entity(m_parent);
            if (entity)
                parent = entity;
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
        
        child->set_parent(NULL);
    }

    void ZstEntityBase::update_URI(const ZstURI& original_path)
    {		
        // Update path in entity lookup
        hierarchy_events()->invoke([this, &original_path](ZstHierarchyAdaptor* adaptor) {
            adaptor->update_entity_URI(this, original_path);
        });
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

	void ZstEntityBase::set_name(const char* name)
	{
        auto orig_path = URI();
        {
            std::lock_guard<std::mutex> lock(m_entity_lock);
            if (parent())
                m_uri = m_uri.parent() + name;
            else
                m_uri = ZstURI(name);
        }
        this->update_URI(orig_path);

        // Refresh URI in parent child lists
        if (this->parent()) {
            auto p = parent();
            p->remove_child(this);
            p->add_child(this);
        }

        // Update new URI across the performance
        m_entity_events->invoke([this, orig_path](ZstEntityAdaptor* adaptor) {
            adaptor->publish_entity_update(this, orig_path);
        });
	}

    void ZstEntityBase::get_child_cables(ZstCableBundle & bundle)
    {
    }

    void ZstEntityBase::get_child_entities(ZstEntityBundle & bundle, bool include_parent, bool recursive, ZstEntityType filter)
    {
        if (include_parent){
            if (filter == ZstEntityType::NONE || this->entity_type() == filter) {
                //std::lock_guard<std::mutex> lock(m_entity_lock);
                bundle.add(this);
            }
        }
    }

    ZstEntityAdaptor* ZstEntityBase::entity_events()
    {
        return m_entity_events->get_default_adaptor().get();
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
			Log::entity(Log::Level::warn, "Entity {} not registered. Can't aquire ownership.", URI().path());
		}

        m_session_events->invoke([this](ZstSessionAdaptor* adaptor) {
			adaptor->aquire_entity_ownership(this);
        });
    }

    void ZstEntityBase::release_ownership()
    {
		if (!is_registered()) {
			Log::entity(Log::Level::warn, "Entity {} not registered. Can't release ownership.", URI().path());
			return;
		}

        m_session_events->invoke([this](ZstSessionAdaptor* adaptor) {
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
			Log::entity(Log::Level::warn, "Entity {} not registered. Can't activate.", URI().path());
			return;
		}

		m_hierarchy_events->invoke([this](ZstHierarchyAdaptor* adaptor) {
			adaptor->activate_entity(this, ZstTransportRequestBehaviour::SYNC_REPLY);
		});
	}

	void ZstEntityBase::activate_async()
	{
		if (!is_registered()) {
			Log::entity(Log::Level::warn, "Entity {} not registered. Can't deactivate.", URI().path());
			return;
		}
		
		m_hierarchy_events->invoke([this](ZstHierarchyAdaptor* adaptor) {
			adaptor->activate_entity(this, ZstTransportRequestBehaviour::ASYNC_REPLY);
		});
	}

	void ZstEntityBase::deactivate()
	{
        if (is_destroyed())
            return;

		if (!is_registered() || !is_activated()) {
			Log::entity(Log::Level::warn, "Entity {} not registered. Can't deactivate.", URI().path());
			return;
		}

		m_hierarchy_events->invoke([this](ZstHierarchyAdaptor* adaptor) {
			adaptor->deactivate_entity(this, ZstTransportRequestBehaviour::SYNC_REPLY);
		});
	}

	void ZstEntityBase::deactivate_async()
	{
		if (!is_registered()) {
			Log::entity(Log::Level::warn, "Entity {} not registered. Can't asynchronously deactivate.", URI().path());
			return;
		}

		m_hierarchy_events->invoke([this](ZstHierarchyAdaptor* adaptor) {
			adaptor->deactivate_entity(this, ZstTransportRequestBehaviour::ASYNC_REPLY);
		});
	}

    void ZstEntityBase::set_entity_type(ZstEntityType entity_type) {
        std::lock_guard<std::mutex> lock(m_entity_lock);
        m_entity_type = entity_type;
    }

    void ZstEntityBase::set_parent(ZstEntityBase *entity) {
        auto orig_path = URI();
        {
            std::lock_guard<std::mutex> lock(m_entity_lock);
            if (entity)
                m_parent = entity->URI();
            else
                m_parent = ZstURI();

            if (m_parent.is_empty()) {
                m_uri = m_uri.last();
            }
            else if (!URI().contains(m_parent)) {
                m_uri = m_parent + m_uri.last();
            }
        }
        this->update_URI(orig_path);
    }

    void ZstEntityBase::dispatch_destroyed()
    {
		if (activation_status() != ZstSyncStatus::DESTROYED && is_registered()) {

			// Set child entities and this entity as destroyed so they won't queue destruction events later
			ZstEntityBundle bundle;
			get_child_entities(bundle);
			for (auto child : bundle) {
				child->set_activation_status(ZstSyncStatus::DESTROYED);
			}

            synchronisable_event_dispatcher()->invoke([this](ZstSynchronisableAdaptor* adaptor) {
                adaptor->on_synchronisable_destroyed(this, true);
            });
        }
    }

    ZST_EXPORT std::shared_ptr<ZstEventDispatcher<ZstEntityAdaptor>>& ZstEntityBase::entity_event_dispatcher()
    {
        return m_entity_events;
    }

    std::shared_ptr<ZstEventDispatcher<ZstSessionAdaptor> > & ZstEntityBase::session_events()
    {
        return m_session_events;
    }

	std::shared_ptr<ZstEventDispatcher<ZstHierarchyAdaptor>>& ZstEntityBase::hierarchy_events()
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
            m_entity_events->invoke([this](ZstEntityAdaptor* adaptor) {
				adaptor->on_entity_registered(this);
			});
			this->on_registered();

			//synchronisable_events()->invoke([this](std::shared_ptr<ZstSynchronisableAdaptor> adaptor) {
			//	adaptor->synchronisable_has_event(this);
			//});
		}
	}
}
