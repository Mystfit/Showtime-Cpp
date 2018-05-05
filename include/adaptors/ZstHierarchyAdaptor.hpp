#pragma once

#include <ZstExports.h>
#include <entities/ZstPerformer.h>
#include <entities/ZstPlug.h>
#include <adaptors/ZstEventAdaptor.hpp>

class ZstHierarchyAdaptor : public ZstEventAdaptor {
public:
	ZST_EXPORT virtual void on_performer_arriving(ZstPerformer * performer);
	ZST_EXPORT virtual void on_performer_leaving(ZstPerformer * performer);

	ZST_EXPORT virtual void on_entity_arriving(ZstEntityBase * entity);
	ZST_EXPORT virtual void on_entity_leaving(ZstEntityBase * entity);

	ZST_EXPORT virtual void on_plug_arriving(ZstPlug * plug);
	ZST_EXPORT virtual void on_plug_leaving(ZstPlug * plug);
};
