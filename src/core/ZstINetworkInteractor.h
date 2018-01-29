#pragma once

#include <entities\ZstPlug.h>

class ZstINetworkInteractor {
public:
	virtual void publish(ZstPlug * plug) = 0;
	virtual void enqueue_synchronisable_event(ZstSynchronisable * synchronisable) = 0;
};