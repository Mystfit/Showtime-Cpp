#pragma once

#include <ZstURI.h>
#include <ZstExports.h>
#include <ZstEventDispatcher.hpp>
#include <adaptors/ZstHierarchyAdaptor.hpp>
#include <entities/ZstEntityBase.h>
#include <entities/ZstPlug.h>
#include <entities/ZstPerformer.h>
#include "liasons/ZstPlugLiason.hpp"
#include "liasons/ZstSynchronisableLiason.hpp"

class ZstHierarchy : 
	public ZstPlugLiason,
	public ZstSynchronisableLiason,
	public ZstSynchronisableAdaptor
{
public:
	ZST_EXPORT ZstHierarchy();
	ZST_EXPORT ~ZstHierarchy();
	ZST_EXPORT virtual void destroy();

	// ------------------------------
	// Activations
	// ------------------------------

	ZST_EXPORT virtual void activate_entity(ZstEntityBase* entity, bool async = false);
	ZST_EXPORT virtual void destroy_entity(ZstEntityBase * entity, bool async = false);
		
	
	// ------------------------------
	// Performers
	// ------------------------------

	ZST_EXPORT virtual void add_performer(ZstPerformer & performer);
	ZST_EXPORT virtual ZstPerformer * get_performer_by_URI(const ZstURI & uri) const;


	// ------------------------------
	// Hierarchy queries
	// ------------------------------

	ZST_EXPORT virtual ZstEntityBase * find_entity(const ZstURI & path);
	ZST_EXPORT virtual ZstPlug * find_plug(const ZstURI & path);

	ZST_EXPORT virtual void add_proxy_entity(ZstEntityBase & entity);
	ZST_EXPORT virtual void remove_proxy_entity(ZstEntityBase * entity);

	// -----------------
	// Event dispatchers
	// -----------------
	
	//ZST_EXPORT ZstEventDispatcher<ZstHierarchyAdaptor*> & synchronisable_events();
	ZST_EXPORT ZstEventDispatcher<ZstHierarchyAdaptor*> & hierarchy_events();
	ZST_EXPORT virtual void process_events();
	ZST_EXPORT virtual void flush_events();

	// -----------------
	// Adaptor overrides
	// -----------------
	ZST_EXPORT void synchronisable_has_event(ZstSynchronisable * synchronisable) override;


protected:
	ZstPerformerMap m_clients;

private:
	ZstEventDispatcher<ZstSynchronisableAdaptor*> m_synchronisable_events;
	ZstEventDispatcher<ZstHierarchyAdaptor*> m_hierarchy_events;
};