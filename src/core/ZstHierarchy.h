#pragma once

#include <ZstURI.h>
#include <ZstExports.h>
#include <ZstEventDispatcher.hpp>
#include <adaptors/ZstSessionAdaptor.hpp>
#include <entities/ZstEntityBase.h>
#include <entities/ZstPlug.h>
#include <entities/ZstPerformer.h>
#include "liasons/ZstSynchronisableLiason.hpp"

class ZstHierarchy : 
	public ZstEventDispatcher<ZstSessionAdaptor*>,
	protected ZstSynchronisableLiason,
	protected ZstSynchronisableAdaptor
{
public:
	// ------------------------------
	// Activations
	// ------------------------------

	ZST_EXPORT virtual void activate_entity(ZstEntityBase* entity, bool async = false);
	ZST_EXPORT virtual void destroy_entity(ZstEntityBase * entity, bool async = false);
	ZST_EXPORT virtual void destroy_plug(ZstPlug * plug, bool async);
	
	
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

protected:
	ZstPerformerMap m_clients;

private:
};