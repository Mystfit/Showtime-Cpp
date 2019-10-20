#pragma once

#include "ZstExports.h"
#include "ZstTransportLayerBase.hpp"
#include "../ZstStageMessage.h"
#include "../adaptors/ZstStageTransportAdaptor.hpp"

namespace showtime 
{
	ZST_CLASS_EXPORTED class ZstStageTransport : 
		public ZstTransportLayerBase<ZstStageMessage, ZstStageTransportAdaptor>,
		public ZstStageTransportAdaptor
	{
	};
}
