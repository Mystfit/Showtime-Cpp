#pragma once

#include <iostream>
#include "ZstExports.h"
#include "ZstURI.h"
#include "ZstCable.h"
#include "ZstCallbackQueue.h"

//Plug arriving/leaving events
class ZstPlug;
class ZstPlugEvent {
public:
	ZST_EXPORT virtual ~ZstPlugEvent() {}
	ZST_EXPORT virtual void run(ZstPlug * plug) {}
};

//Entity arriving/leaving events
class ZstEntityBase;
class ZstComponentEvent {
public:
	ZST_EXPORT virtual ~ZstComponentEvent() {}
	ZST_EXPORT virtual void run(ZstEntityBase* entity) {}
};

//Entity type arriving/leaving events
class ZstComponentTypeEvent {
public:
	ZST_EXPORT virtual ~ZstComponentTypeEvent() {}
	ZST_EXPORT virtual void run(ZstEntityBase* entity) {}
};

//Cable arriving/leaving events
class ZstCableEvent {
public:
	ZST_EXPORT virtual ~ZstCableEvent() {}
	ZST_EXPORT virtual void run(ZstCable * cable) {}
};
