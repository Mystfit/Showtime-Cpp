#pragma once

#include "ZstExports.h"
#include "entities/ZstComponent.h"
#include "entities/ZstPlug.h"
#include "adaptors/ZstEventAdaptor.hpp"

class ZstComputeAdaptor : public ZstEventAdaptor {
public:
	ZST_EXPORT virtual void on_compute(ZstComponent * component, ZstInputPlug * plug);
};
