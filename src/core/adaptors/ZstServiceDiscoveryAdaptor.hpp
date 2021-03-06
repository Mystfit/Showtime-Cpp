#pragma once

#include "ZstTransportAdaptor.hpp"
#include "../ZstServerBeaconMessage.h"

namespace showtime {

	class ZST_CLASS_EXPORTED ZstServiceDiscoveryAdaptor : public ZstTransportAdaptor
	{
	public:
		MULTICAST_DELEGATE_OneParam(ZST_EXPORT, receive_msg, const std::shared_ptr<ZstServerBeaconMessage>&, msg)
	};
}
