#pragma once

#include "entities\ZstPlug.h"

class ZstGraphSender {
public:
	virtual void publish(ZstPlug * plug) = 0;
};