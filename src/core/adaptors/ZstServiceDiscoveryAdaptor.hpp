#pragma once

#include "ZstTransportAdaptor.hpp"
#include "../ZstServerBeaconMessage.h"

namespace showtime {

	class ZST_CLASS_EXPORTED ZstServiceDiscoveryAdaptor : public ZstTransportAdaptor
	{
	public:
		ZST_EXPORT virtual void on_receive_msg(const std::shared_ptr<ZstServerBeaconMessage>& msg);
	};
}
