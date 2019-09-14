#pragma once

#include "ZstExports.h"
#include "entities/ZstComponent.h"
#include "entities/ZstPlug.h"
#include "adaptors/ZstEventAdaptor.hpp"

namespace showtime {

class ZST_CLASS_EXPORTED ZstComputeAdaptor : 
	public ZstEventAdaptor
{
public:
	ZST_EXPORT virtual void on_compute(ZstComponent * component, ZstInputPlug * plug);
};

}
