#pragma once

#include <memory>
#include "ZstExports.h"
#include "ZstCable.h"
#include "entities/ZstPlug.h"
#include "../ZstValue.h"

namespace showtime {

class ZstPlugLiason {
public:
	ZST_EXPORT void plug_remove_cable(ZstPlug * plug, ZstCable * cable);
	ZST_EXPORT void plug_add_cable(ZstPlug * plug, ZstCable * cable);
	ZST_EXPORT void output_plug_set_can_fire(ZstOutputPlug* plug, bool can_fire);
	ZST_EXPORT void output_plug_set_transport(ZstOutputPlug * plug, std::shared_ptr<ZstTransportAdaptor> & transport);
};

};
