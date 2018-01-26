#pragma once

#include "entities\ZstPlug.h"

class ZstINetworkInteractor {
public:
	virtual void publish(ZstPlug * plug) = 0;
	virtual void queue_synchronisable_activation(ZstSynchronisable * synchronisable) = 0;
	virtual void queue_synchronisable_deactivation(ZstSynchronisable * synchronisable) = 0;
};