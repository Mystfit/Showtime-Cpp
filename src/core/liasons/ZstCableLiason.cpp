#include "ZstCableLiason.hpp"

namespace showtime {

void ZstCableLiason::cable_set_local(ZstCable * cable)
{
	cable->set_proxy();
}

}
