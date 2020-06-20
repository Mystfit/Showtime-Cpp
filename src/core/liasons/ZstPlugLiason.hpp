#pragma once

#include <memory>
#include <showtime/ZstExports.h>
#include <showtime/ZstCable.h>
#include <showtime/entities/ZstPlug.h>
#include "../adaptors/ZstGraphTransportAdaptor.hpp"

namespace showtime {

class ZstPlugLiason {
public:
	ZST_EXPORT void plug_remove_cable(ZstPlug * plug, ZstCable * cable);
	ZST_EXPORT void plug_add_cable(ZstPlug * plug, ZstCable * cable);
	ZST_EXPORT void output_plug_set_can_fire(ZstOutputPlug* plug, bool can_fire);
	ZST_EXPORT void output_plug_set_transport(ZstOutputPlug * plug, std::shared_ptr<ZstGraphTransportAdaptor> transport);
};

};
