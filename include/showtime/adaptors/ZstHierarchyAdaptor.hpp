#pragma once

#include "ZstExports.h"
#include "ZstEventAdaptor.hpp"
#include "ZstConstants.h"

namespace showtime {

	// Forwards
	class ZstPerformer;
	class ZstEntityBase;
	class ZstPlug;
	class ZstEntityFactory;

	class ZST_CLASS_EXPORTED ZstHierarchyAdaptor : 
		public ZstEventAdaptor
	{
	public:
		// Outgoing events

		ZST_EXPORT virtual void on_performer_arriving(ZstPerformer * performer);
		ZST_EXPORT virtual void on_performer_leaving(const ZstURI& performer_path);

		ZST_EXPORT virtual void on_entity_arriving(ZstEntityBase * entity);
		ZST_EXPORT virtual void on_entity_leaving(const ZstURI& entity_path);
		ZST_EXPORT virtual void on_entity_updated(ZstEntityBase* entity);

		ZST_EXPORT virtual void on_factory_arriving(ZstEntityFactory * factory);
		ZST_EXPORT virtual void on_factory_leaving(const ZstURI& factory_path);


		// Interface events 

		ZST_EXPORT virtual void activate_entity(ZstEntityBase* entity, const ZstTransportRequestBehaviour& sendtype);
		ZST_EXPORT virtual void deactivate_entity(ZstEntityBase* entity, const ZstTransportRequestBehaviour& sendtype);
		ZST_EXPORT virtual ZstEntityBase* find_entity(const ZstURI& path) const;
		ZST_EXPORT virtual void update_entity_URI(ZstEntityBase* entity, const ZstURI& original_path);
		ZST_EXPORT virtual ZstPerformer* get_local_performer() const;
	};

}
