#pragma once

#include <mutex>

#include <showtime/ZstURI.h>
#include <showtime/ZstExports.h>
#include <showtime/entities/ZstEntityFactory.h>
#include <showtime/entities/ZstEntityBase.h>
#include <showtime/entities/ZstPlug.h>
#include <showtime/entities/ZstPerformer.h>
#include <showtime/adaptors/ZstHierarchyAdaptor.hpp>
#include <showtime/adaptors/ZstEntityAdaptor.hpp>

#include "ZstEventDispatcher.hpp"
#include "ZstSynchronisableModule.h"
#include "liasons/ZstEntityFactoryLiason.hpp"
#include "liasons/ZstEntityLiason.hpp"
#include "liasons/ZstPlugLiason.hpp"
#include "liasons/ZstSynchronisableLiason.hpp"
#include "adaptors/ZstTransportAdaptor.hpp"

namespace showtime {

class ZST_CLASS_EXPORTED ZstHierarchy :
	public ZstSynchronisableModule,
	public ZstPlugLiason,
	public ZstEntityLiason,
	public ZstHierarchyAdaptor,
	public ZstEntityFactoryLiason,
	public ZstEntityAdaptor
{
	using ZstEntityAdaptor::weak_from_this;
	using ZstSynchronisableAdaptor::weak_from_this;
public:
	ZST_EXPORT ZstHierarchy();
	ZST_EXPORT ~ZstHierarchy();
	ZST_EXPORT virtual void init_adaptors() override;

	// ------------------------------
	// Activations
	// ------------------------------

	ZST_EXPORT void register_entity(ZstEntityBase* entity);
	ZST_EXPORT virtual void activate_entity(ZstEntityBase* entity, const ZstTransportRequestBehaviour & sendtype = ZstTransportRequestBehaviour::SYNC_REPLY) override;
	ZST_EXPORT virtual void deactivate_entity(ZstEntityBase * entity, const ZstTransportRequestBehaviour & sendtype = ZstTransportRequestBehaviour::SYNC_REPLY) override;
	ZST_EXPORT virtual ZstEntityBase * create_entity(const ZstURI & creatable_path, const char * name, const ZstTransportRequestBehaviour & sendtype = ZstTransportRequestBehaviour::SYNC_REPLY);

	// ------------------------------
	// Performers
	// ------------------------------
    
	ZST_EXPORT virtual void get_performers(ZstEntityBundle & bundle) const;
	ZST_EXPORT virtual void get_performers(ZstEntityBundle* bundle) const;
    ZST_EXPORT virtual ZstPerformer * get_local_performer() const override = 0;


	// ------------------------------
	// Hierarchy queries
	// ------------------------------

	ZST_EXPORT virtual void update_entity_URI(ZstEntityBase* entity, const ZstURI& original_path) override;
	ZST_EXPORT virtual ZstEntityBase * find_entity(const ZstURI & path) const override;
	ZST_EXPORT virtual ZstEntityBase * walk_to_entity(const ZstURI & path) const;


	// ------------------------------
	// Hierarchy manipulation
	// ------------------------------

	ZST_EXPORT virtual std::unique_ptr<ZstEntityBase> create_proxy_entity(EntityTypes entity_type, const EntityData* entity_data, const void* payload);
	ZST_EXPORT virtual void add_proxy_entity(std::unique_ptr<ZstEntityBase> entity);
	ZST_EXPORT virtual void update_proxy_entity(ZstEntityBase * original, EntityTypes entity_type, const EntityData* entity_data, const void* payload);
	ZST_EXPORT virtual void remove_proxy_entity(ZstEntityBase * entity);
	ZST_EXPORT const EntityData* get_entity_field(EntityTypes entity_type, const void* data);
    ZST_EXPORT std::unique_ptr<ZstEntityBase> unpack_entity(EntityTypes entity_type, const void* entity_data);

	
	// -----------------
	// Event dispatchers
	// -----------------
	
	ZST_EXPORT std::shared_ptr<ZstEventDispatcher<ZstHierarchyAdaptor> > & hierarchy_events();
	ZST_EXPORT virtual void process_events() override;
	ZST_EXPORT virtual void flush_events() override;


	// -----------------
	// Adaptor overrides
	// -----------------

	ZST_EXPORT void on_synchronisable_destroyed(ZstSynchronisable * synchronisable, bool already_removed) override;
	ZST_EXPORT virtual void on_register_entity(ZstEntityBase * entity) override;
	ZST_EXPORT virtual void register_entity_tick(ZstEntityBase* entity) override;
	ZST_EXPORT virtual void unregister_entity_tick(ZstEntityBase* entity) override;

    
    // ------------------------------
    // Entity lookups
    // ------------------------------

    ZST_EXPORT virtual void add_entity_to_lookup(ZstEntityBase * entity);
    ZST_EXPORT virtual void remove_entity_from_lookup(const ZstURI & entity);
	ZST_EXPORT virtual void update_entity_in_lookup(ZstEntityBase* entity, const ZstURI& new_path);
	

protected:
	// ------------------------------
	// Activations
	// ------------------------------

	ZST_EXPORT virtual void activate_entity_complete(ZstEntityBase * entity);
	ZST_EXPORT virtual void destroy_entity_complete(ZstEntityBase * entity);
    ZST_EXPORT void dispatch_entity_arrived_event(ZstEntityBase * entity);

private:
	void reaper_cleanup_entity(ZstEntityBase* entity);
	std::shared_ptr<ZstEventDispatcher<ZstHierarchyAdaptor> > m_hierarchy_events;
	std::recursive_mutex m_hierarchy_mutex;
	ZstEntityMap m_entity_lookup;
	std::set< std::unique_ptr<ZstSynchronisable> > m_proxies;
	std::set< ZstEntityBase* > m_ticking_entities;
};

}
