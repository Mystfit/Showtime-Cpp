#pragma once

#include <ZstExports.h>

//Forwards
class ZstSynchronisable;
class ZstEntityBase;
class ZstComponent;
class ZstContainer;
class ZstPerformer;
class ZstPlug;
class ZstCable;

// -------------------
// Base event template
// -------------------

template<typename EventTarget>
class ZstEvent {
public:
	ZstEvent(EventTarget target) {
		m_target = target;
	}

	int id() { return m_id; }
	EventTarget target() { return m_target; }
	
private:
	int m_id;
	EventTarget m_target;
};


// ---------------------------
// Events
// ---------------------------

class ZstSynchronisableEvent : public ZstEvent<ZstSynchronisable*>
{
public:
	ZstSynchronisableEvent(ZstSynchronisable * target);
};


class ZstEntityEvent : public ZstEvent<ZstEntityBase*>
{
public:
	ZstEntityEvent(ZstEntityBase * target);
};


class ZstComponentEvent : public ZstEvent<ZstComponent*>
{
public:
	ZstComponentEvent(ZstComponent * target);
};


class ZstContainerEvent : public ZstEvent<ZstContainer*>
{
public:
	ZstContainerEvent(ZstContainer * target);
};


class ZstPerformerEvent : public ZstEvent<ZstPerformer*>
{
public:
	ZstPerformerEvent(ZstPerformer * target);
};


class ZstPlugEvent : public ZstEvent<ZstPlug*>
{
public:
	ZstPlugEvent(ZstPlug * target);
};


class ZstCableEvent : public ZstEvent<ZstCable*>
{
public:
	ZstCableEvent(ZstCable * target);
};
