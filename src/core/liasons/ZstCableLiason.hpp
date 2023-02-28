#pragma once

#include <showtime/ZstExports.h>
#include <showtime/ZstCable.h>

namespace showtime {

class ZstCableLiason {
public:
	ZST_EXPORT void cable_set_local(ZstCable * cable);
	ZST_EXPORT void cable_update_address(ZstCable* cable, const ZstCableAddress& address);
};

}
