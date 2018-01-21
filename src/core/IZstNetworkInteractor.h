#pragma once

#include "entities\ZstPlug.h"

class IZstNetworkInteractor {
public:
	virtual void publish(ZstPlug * plug) = 0;
};