#pragma once

#include <iostream>
#include "ZstExports.h"
#include "ZstURI.h"
#include "ZstCable.h"

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
class ZstEntityBase;
class ZstEntityEventCallback {
public:
	ZST_EXPORT virtual ~ZstEntityEventCallback() {}
	ZST_EXPORT virtual void run(ZstEntityBase* entity) {}
};

//Entity template arriving/leaving events
class ZstEntityTemplateEventCallback {
public:
    ZST_EXPORT virtual ~ZstEntityTemplateEventCallback() {}
    ZST_EXPORT virtual void run(ZstEntityBase* entity_template) {}
};

//Cable arriving/leaving events
class ZstCableEventCallback {
public:
	ZST_EXPORT virtual ~ZstCableEventCallback() {}
	ZST_EXPORT virtual void run(ZstCable cable) {}
};
