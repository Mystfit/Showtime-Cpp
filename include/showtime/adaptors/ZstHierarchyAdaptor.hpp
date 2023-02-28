#pragma once

#include <showtime/ZstConstants.h>
#include <showtime/ZstExports.h>
#include "ZstEventAdaptor.hpp"

namespace showtime {

	// Forwards
	class ZstPerformer;
	class ZstEntityBase;
	class ZstPlug;
	class ZstEntityFactory;

	class ZST_CLASS_EXPORTED ZstHierarchyAdaptor
#ifndef SWIG
		: public inheritable_enable_shared_from_this< ZstHierarchyAdaptor >
#endif
	{
	public:
		ZST_EXPORT ZstHierarchyAdaptor();
		// Outgoing events
		MULTICAST_DELEGATE_OneParam(ZST_EXPORT, performer_arriving, ZstPerformer*, performer)
		MULTICAST_DELEGATE_OneParam(ZST_EXPORT, performer_leaving, ZstPerformer*, performer)
		MULTICAST_DELEGATE_OneParam(ZST_EXPORT, entity_arriving, ZstEntityBase*, entity)
		MULTICAST_DELEGATE_OneParam(ZST_EXPORT, entity_leaving, ZstEntityBase*, entity, entity)
		MULTICAST_DELEGATE_TwoParams(ZST_EXPORT, entity_updated, ZstEntityBase*, entity, const ZstURI&, orig_path)
		MULTICAST_DELEGATE_OneParam(ZST_EXPORT, factory_arriving, ZstEntityFactory*, factory)
		MULTICAST_DELEGATE_OneParam(ZST_EXPORT, factory_leaving, ZstEntityFactory*, factory)


		// Interface events 
		ZST_EXPORT virtual void activate_entity(ZstEntityBase * entity, const ZstTransportRequestBehaviour & sendtype) {};
		ZST_EXPORT virtual void deactivate_entity(ZstEntityBase* entity, const ZstTransportRequestBehaviour& sendtype) {};
		ZST_EXPORT virtual ZstEntityBase* find_entity(const ZstURI& path) const { return nullptr; };
		ZST_EXPORT virtual void update_entity_URI(ZstEntityBase* entity, const ZstURI& original_path) {};
		ZST_EXPORT virtual ZstPerformer* get_local_performer() const { return nullptr; };
		ZST_EXPORT virtual void register_entity_tick(ZstEntityBase* entity) {};
		ZST_EXPORT virtual void unregister_entity_tick(ZstEntityBase* entity) {};
	};

}
