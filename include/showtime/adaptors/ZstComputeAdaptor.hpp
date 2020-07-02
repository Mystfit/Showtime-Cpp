#pragma once

#include <showtime/ZstExports.h>
#include <showtime/entities/ZstComponent.h>
#include <showtime/entities/ZstPlug.h>
#include <showtime/adaptors/ZstEventAdaptor.hpp>

namespace showtime {
	class ZST_CLASS_EXPORTED ZstComputeAdaptor : 
		public ZstEventAdaptor
	{
	public:
		MULTICAST_DELEGATE_TwoParams(Compute, ZstComponent*, ZstInputPlug*)
		ZST_EXPORT virtual void on_compute(ZstComponent * component, ZstInputPlug * plug);
	};
}
