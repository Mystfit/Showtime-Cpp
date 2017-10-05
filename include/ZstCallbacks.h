#pragma once

#include <iostream>
#include "ZstExports.h"
#include "ZstURI.h"
#include "ZstCable.h"
#include "ZstEvent.h"
#include "entities/ZstEntityBase.h"

//Callback for incoming plug data events
class ZstInputPlug;
class ZstPlugDataEventCallback {
public:
	ZST_EXPORT virtual ~ZstPlugDataEventCallback() {}
	ZST_EXPORT virtual void run(ZstInputPlug * e) {}
};

//Plug arriving/leaving events
class ZstPlugEventCallback {
public:
	ZST_EXPORT virtual ~ZstPlugEventCallback() {}
	ZST_EXPORT virtual void run(ZstURI plug) {}
};

//Entity arriving/leaving events
class ZstEntityEventCallback {
public:
	ZST_EXPORT virtual ~ZstEntityEventCallback() {}
	ZST_EXPORT virtual void run(ZstEntityBase* entity) {}
};

//Cable arriving/leaving events
class ZstCableEventCallback {
public:
	ZST_EXPORT virtual ~ZstCableEventCallback() {}
	ZST_EXPORT virtual void run(ZstCable cable) {}
};

//Compute events
class ZstInputPlug;
class ZstComponent;
class ZstComputeCallback : public ZstPlugDataEventCallback {
public:
    ZST_EXPORT void set_target_filter(ZstComponent * component);
    ZST_EXPORT void run(ZstInputPlug * plug) override;
private:
    ZstComponent * m_component;
};
