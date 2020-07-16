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
		// Outgoing events
		MULTICAST_DELEGATE_OneParam(ZST_EXPORT, performer_arriving, ZstPerformer*, performer)
		MULTICAST_DELEGATE_OneParam(ZST_EXPORT, performer_leaving, const ZstURI&, performer_path)
		MULTICAST_DELEGATE_OneParam(ZST_EXPORT, entity_arriving, ZstEntityBase*, entity)
		MULTICAST_DELEGATE_OneParam(ZST_EXPORT, entity_leaving, const ZstURI&, entity_path)
		MULTICAST_DELEGATE_OneParam(ZST_EXPORT, entity_updated, ZstEntityBase*, entity)
		MULTICAST_DELEGATE_OneParam(ZST_EXPORT, factory_arriving, ZstEntityFactory*, factory)
		MULTICAST_DELEGATE_OneParam(ZST_EXPORT, factory_leaving, const ZstURI&, factory_path)


		// Interface events 
		ZST_EXPORT virtual void activate_entity(ZstEntityBase* entity, const ZstTransportRequestBehaviour& sendtype);
		ZST_EXPORT virtual void deactivate_entity(ZstEntityBase* entity, const ZstTransportRequestBehaviour& sendtype);
		ZST_EXPORT virtual ZstEntityBase* find_entity(const ZstURI& path) const;
		ZST_EXPORT virtual void update_entity_URI(ZstEntityBase* entity, const ZstURI& original_path);
		ZST_EXPORT virtual ZstPerformer* get_local_performer() const;
	};

}
