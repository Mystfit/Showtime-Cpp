#pragma once

#include <ZstExports.h>
#include <adaptors/ZstEventAdaptor.hpp>
#include <entities/ZstPlug.h>
#include <entities/ZstEntityBase.h>
#include <entities/ZstPerformer.h>
#include <ZstCable.h>

class ZstSessionAdaptor : public ZstEventAdaptor {
public:
	ZST_EXPORT virtual ~ZstSessionAdaptor() {};
	ZST_EXPORT virtual void on_connected_to_stage();
	ZST_EXPORT virtual void on_disconnected_from_stage();

	ZST_EXPORT virtual void on_cable_created(ZstCable * cable);
	ZST_EXPORT virtual void on_cable_destroyed(ZstCable * cable);
};
