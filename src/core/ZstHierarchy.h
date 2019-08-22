#pragma once

#include <mutex>

#include <ZstURI.h>
#include "ZstExports.h"
#include "entities/ZstEntityFactory.h"
#include "entities/ZstEntityBase.h"
#include "entities/ZstPlug.h"
#include "entities/ZstPerformer.h"
#include "adaptors/ZstHierarchyAdaptor.hpp"
#include "adaptors/ZstEntityAdaptor.hpp"

#include "ZstEventDispatcher.hpp"
#include "ZstSynchronisableModule.h"
#include "liasons/ZstEntityFactoryLiason.hpp"
#include "liasons/ZstPlugLiason.hpp"
#include "liasons/ZstSynchronisableLiason.hpp"
#include "adaptors/ZstTransportAdaptor.hpp"


class ZST_EXPORT ZstHierarchy : 
	public ZstSynchronisableModule,
	public ZstPlugLiason,
	public ZstEntityFactoryLiason,
	public ZstEntityAdaptor
{
public:
	ZST_EXPORT ZstHierarchy();
	ZST_EXPORT ~ZstHierarchy();
    
	ZST_EXPORT virtual void destroy() override;

	// ------------------------------
	// Activations
	// ------------------------------

	ZST_EXPORT virtual void activate_entity(ZstEntityBase* entity, const ZstTransportSendType & sendtype = ZstTransportSendType::SYNC_REPLY);
	ZST_EXPORT virtual void destroy_entity(ZstEntityBase * entity, const ZstTransportSendType & sendtype = ZstTransportSendType::SYNC_REPLY);
	ZST_EXPORT virtual ZstEntityBase * create_entity(const ZstURI & creatable_path, const char * name, const ZstTransportSendType & sendtype = ZstTransportSendType::SYNC_REPLY);

	// ------------------------------
	// Performers
	// ------------------------------
    
	ZST_EXPORT virtual void add_performer(const ZstPerformer & performer);
	ZST_EXPORT virtual ZstPerformer * get_performer_by_URI(const ZstURI & uri) const;
	ZST_EXPORT virtual ZstEntityBundle & get_performers(ZstEntityBundle & bundle) const;
    ZST_EXPORT virtual ZstPerformer * get_local_performer() const = 0;


	// ------------------------------
	// Hierarchy queries
	// ------------------------------

	ZST_EXPORT virtual ZstEntityBase * find_entity(const ZstURI & path) const;
	ZST_EXPORT virtual ZstEntityBase * walk_to_entity(const ZstURI & path) const;


	// ------------------------------
	// Hierarchy manipulation
	// ------------------------------

	ZST_EXPORT virtual ZstMsgKind add_proxy_entity(const ZstEntityBase & entity);
	ZST_EXPORT virtual ZstMsgKind update_proxy_entity(const ZstEntityBase & entity);
	ZST_EXPORT virtual ZstMsgKind remove_proxy_entity(ZstEntityBase * entity);

	
	// -----------------
	// Event dispatchers
	// -----------------
	
	ZST_EXPORT ZstEventDispatcher<ZstHierarchyAdaptor*> & hierarchy_events();
	ZST_EXPORT virtual void process_events() override;
	ZST_EXPORT virtual void flush_events() override;

	// -----------------
	// Adaptor overrides
	// -----------------
	ZST_EXPORT void on_synchronisable_destroyed(ZstSynchronisable * synchronisable) override;
	ZST_EXPORT virtual void on_register_entity(ZstEntityBase * entity) override;

protected:
	// ------------------------------
	// Activations
	// ------------------------------

	ZST_EXPORT virtual void activate_entity_complete(ZstEntityBase * entity);
	ZST_EXPORT virtual void destroy_entity_complete(ZstEntityBase * entity);
    ZST_EXPORT void dispatch_entity_arrived_event(ZstEntityBase * entity);



	// ------------------------------
	// Entity lookups
	// ------------------------------

	ZST_EXPORT virtual void add_entity_to_lookup(ZstEntityBase * entity);
	ZST_EXPORT virtual void remove_entity_from_lookup(ZstEntityBase * entity);
	
	//Client map
	ZstPerformerMap m_clients;

private:
	ZstEventDispatcher<ZstSynchronisableAdaptor*> m_synchronisable_events;
	ZstEventDispatcher<ZstHierarchyAdaptor*> m_hierarchy_events;
	std::mutex m_hierarchy_mutex;
	ZstEntityMap m_entity_lookup;
};
