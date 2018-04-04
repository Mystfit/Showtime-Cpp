#pragma once

#include <ZstEvents.h>

class ZstSynchronisableDeferredEvent : public ZstSynchronisableEvent {
	virtual void run(ZstSynchronisable * target) override;
};


class ZstComputeEvent : public ZstInputPlugEvent {
	virtual void run(ZstInputPlug * target) override;
};
