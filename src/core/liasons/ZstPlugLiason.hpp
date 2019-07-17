#pragma once

#include "ZstExports.h"
#include "ZstCable.h"
#include "entities/ZstPlug.h"
#include "../ZstValue.h"

class ZstPlugLiason {
public:
	ZST_EXPORT void plug_remove_cable(ZstPlug * plug, ZstCable * cable);
	ZST_EXPORT void plug_add_cable(ZstPlug * plug, ZstCable * cable);
	ZST_EXPORT void output_plug_set_fire_control_owner(ZstOutputPlug* plug, const ZstURI & owner);
	ZST_EXPORT void output_plug_set_transport(ZstOutputPlug * plug, ZstTransportAdaptor * transport);
};
