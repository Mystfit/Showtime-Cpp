#pragma once

#include <entities/ZstPlug.h>

class ZstINetworkInteractor {
public:
	virtual void send_to_performance(ZstPlug * plug) = 0;
	virtual void enqueue_synchronisable_event(ZstSynchronisable * synchronisable) = 0;
};
