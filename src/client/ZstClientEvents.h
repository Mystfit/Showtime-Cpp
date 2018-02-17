#pragma once

#include <ZstEvents.h>

class ZstSynchronisableDeferredEvent : public ZstSynchronisableEvent {
	virtual void run(ZstSynchronisable * target) override;
};


class ZstEntityLeavingEvent : public ZstEntityEvent {
	virtual void run(ZstEntityBase * target) override;
};


class ZstPlugLeavingEvent : public ZstPlugEvent {
	virtual void run(ZstPlug * target) override;
};


class ZstCableLeavingEvent : public ZstCableEvent {
	virtual void run(ZstCable * target) override;
};


class ZstComputeEvent : public ZstInputPlugEvent {
	virtual void run(ZstInputPlug * target) override;
};
