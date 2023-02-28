#include "ZstCableLiason.hpp"

namespace showtime {

void ZstCableLiason::cable_set_local(ZstCable * cable)
{
	cable->set_proxy();
}

void ZstCableLiason::cable_update_address(ZstCable* cable, const ZstCableAddress& address)
{
	cable->update_address(address);
}

}
