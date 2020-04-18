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
		ZST_EXPORT virtual void on_performer_arriving(ZstPerformer * performer);
		ZST_EXPORT virtual void on_performer_leaving(ZstPerformer * performer);

		ZST_EXPORT virtual void on_entity_arriving(ZstEntityBase * entity);
		ZST_EXPORT virtual void on_entity_leaving(ZstEntityBase * entity);
		ZST_EXPORT virtual void on_entity_updated(ZstEntityBase* entity);

		//ZST_EXPORT virtual void on_plug_arriving(ZstPlug * plug);
		//ZST_EXPORT virtual void on_plug_leaving(ZstPlug * plug);

		ZST_EXPORT virtual void on_factory_arriving(ZstEntityFactory * factory);
		ZST_EXPORT virtual void on_factory_leaving(ZstEntityFactory * factory);

		ZST_EXPORT virtual void activate_entity(ZstEntityBase* entity, const ZstTransportRequestBehaviour& sendtype);
		ZST_EXPORT virtual void deactivate_entity(ZstEntityBase* entity, const ZstTransportRequestBehaviour& sendtype);
		ZST_EXPORT virtual ZstEntityBase* find_entity(const ZstURI& path) const;
		ZST_EXPORT virtual void update_entity_URI(ZstEntityBase* entity, const ZstURI& original_path);

		ZST_EXPORT virtual ZstPerformer* get_local_performer() const;
	};

}
