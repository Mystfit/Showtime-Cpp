#pragma once

#include <ZstURI.h>
#include <ZstExports.h>
#include <ZstEventDispatcher.hpp>
#include <entities/ZstEntityBase.h>
#include <entities/ZstPlug.h>
#include <entities/ZstPerformer.h>
#include <adaptors/ZstHierarchyAdaptor.hpp>
#include <adaptors/ZstEntityAdaptor.hpp>

#include "ZstModule.h"
#include "liasons/ZstPlugLiason.hpp"
#include "liasons/ZstSynchronisableLiason.hpp"
#include "adaptors/ZstTransportAdaptor.hpp"


class ZstHierarchy : 
	public ZstModule,
	public ZstPlugLiason,
	public ZstSynchronisableLiason,
	public ZstSynchronisableAdaptor,
	public ZstEntityAdaptor
{
public:
	ZST_EXPORT ZstHierarchy();
	ZST_EXPORT ~ZstHierarchy();

	ZST_EXPORT virtual void init() override;
	ZST_EXPORT virtual void destroy() override;

	// ------------------------------
	// Activations
	// ------------------------------

	ZST_EXPORT virtual void activate_entity(ZstEntityBase* entity, const ZstTransportSendType & sendtype = ZstTransportSendType::SYNC_REPLY);
	ZST_EXPORT virtual void destroy_entity(ZstEntityBase * entity, const ZstTransportSendType & sendtype = ZstTransportSendType::SYNC_REPLY);
	

	// ------------------------------
	// Performers
	// ------------------------------

	ZST_EXPORT virtual void add_performer(const ZstPerformer & performer);
	ZST_EXPORT virtual ZstPerformer * get_performer_by_URI(const ZstURI & uri) const;
	ZST_EXPORT virtual std::vector<ZstPerformer*> get_performers();


	// ------------------------------
	// Hierarchy queries
	// ------------------------------

	ZST_EXPORT virtual ZstEntityBase * find_entity(const ZstURI & path);
	ZST_EXPORT virtual ZstPlug * find_plug(const ZstURI & path);


	// ------------------------------
	// Hierarchy manipulation
	// ------------------------------

	ZST_EXPORT virtual ZstMsgKind add_proxy_entity(const ZstEntityBase & entity);
	ZST_EXPORT virtual ZstMsgKind remove_proxy_entity(ZstEntityBase * entity);


	// -----------------
	// Event dispatchers
	// -----------------
	
	ZST_EXPORT ZstEventDispatcher<ZstHierarchyAdaptor*> & hierarchy_events();
	ZST_EXPORT virtual void process_events();
	ZST_EXPORT virtual void flush_events();

	// -----------------
	// Adaptor overrides
	// -----------------
	ZST_EXPORT void synchronisable_has_event(ZstSynchronisable * synchronisable) override;
	ZST_EXPORT void on_synchronisable_destroyed(ZstSynchronisable * synchronisable) override;


protected:
	ZST_EXPORT virtual void destroy_entity_complete(ZstEntityBase * entity);
	ZstPerformerMap m_clients;

private:
	ZstEventDispatcher<ZstSynchronisableAdaptor*> m_synchronisable_events;
	ZstEventDispatcher<ZstHierarchyAdaptor*> m_hierarchy_events;
};