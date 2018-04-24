#pragma once

#include <ZstExports.h>
#include <adaptors/ZstEventAdaptor.hpp>
#include <entities/ZstPlug.h>
#include <entities/ZstEntityBase.h>
#include <entities/ZstPerformer.h>
#include <ZstCable.h>

class ZstSessionAdaptor : public ZstEventAdaptor {
public:
	ZST_CLIENT_EXPORT virtual void on_connected_to_stage();
	ZST_CLIENT_EXPORT virtual void on_disconnected_from_stage();

	ZST_CLIENT_EXPORT virtual void on_performer_arriving(ZstPerformer * performer);
	ZST_CLIENT_EXPORT virtual void on_performer_leaving(ZstPerformer * performer);

	ZST_CLIENT_EXPORT virtual void on_entity_arriving(ZstEntityBase * entity);
	ZST_CLIENT_EXPORT virtual void on_entity_leaving(ZstEntityBase * entity);

	ZST_CLIENT_EXPORT virtual void on_plug_arriving(ZstPlug * plug);
	ZST_CLIENT_EXPORT virtual void on_plug_leaving(ZstPlug * plug);
	ZST_CLIENT_EXPORT virtual void on_plug_received_value(ZstInputPlug * plug);

	ZST_CLIENT_EXPORT virtual void on_cable_created(ZstCable * cable);
	ZST_CLIENT_EXPORT virtual void on_cable_destroyed(ZstCable * cable);
};
