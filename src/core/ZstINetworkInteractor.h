#pragma once

#include "entities\ZstPlug.h"

class ZstINetworkInteractor {
public:
	virtual void publish(ZstPlug * plug) = 0;
};