#pragma once

#include <showtime/ZstExports.h>
#include <showtime/entities/ZstComponent.h>
#include <showtime/entities/ZstPlug.h>
#include <showtime/adaptors/ZstEventAdaptor.hpp>

namespace showtime {
	class ZST_CLASS_EXPORTED ZstComputeAdaptor 
#ifndef SWIG
		: public inheritable_enable_shared_from_this< ZstComputeAdaptor >
#endif
	{
	public:
		MULTICAST_DELEGATE_TwoParams(ZST_EXPORT, compute, ZstComponent*, component, ZstInputPlug*, plug)
	};
}
