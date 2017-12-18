#pragma once

#include <iostream>
#include "ZstExports.h"

class ZstEvent {
public:
	ZstEvent() : m_num_calls(0) {}
	int num_calls() const { return m_num_calls; }
	void reset_num_calls() { m_num_calls = 0; }
private:
	int m_num_calls;
};

//Plug arriving/leaving events
class ZstPlug;
class ZstPlugEvent : public ZstEvent {
public:
	ZST_EXPORT virtual ~ZstPlugEvent() {}
	ZST_EXPORT virtual void run(ZstPlug * plug) {}
};

//Entity arriving/leaving events
class ZstEntityBase;
class ZstComponentEvent : public ZstEvent {
public:
	ZST_EXPORT virtual ~ZstComponentEvent() {}
	ZST_EXPORT virtual void run(ZstEntityBase* entity) {}
};

//Entity type arriving/leaving events
class ZstComponentTypeEvent : public ZstEvent {
public:
	ZST_EXPORT virtual ~ZstComponentTypeEvent() {}
	ZST_EXPORT virtual void run(ZstEntityBase* entity) {}
};

//Cable arriving/leaving events
class ZstCable;
class ZstCableEvent : public ZstEvent {
public:
	ZST_EXPORT virtual ~ZstCableEvent() {}
	ZST_EXPORT virtual void run(ZstCable * cable) {}
};
