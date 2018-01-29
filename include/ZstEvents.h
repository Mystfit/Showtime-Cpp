#pragma once

#include <ZstExports.h>

//Enums
//-----

enum ZstEventAction {
	ARRIVING = 0,
	LEAVING
};


//Base callbacks
//----------------

class ZstSynchronisable;
class ZstEvent {
public:
	ZST_EXPORT ZstEvent();
	ZST_EXPORT int num_calls() const;
	ZST_EXPORT void reset_num_calls();
	ZST_EXPORT void increment_calls();
	ZST_EXPORT virtual void cast_run(ZstSynchronisable * target) = 0;
private:
	int m_num_calls;
};

class ZstSynchronisable;
class ZstSynchronisableEvent : public ZstEvent {
public:
	ZST_EXPORT virtual ~ZstSynchronisableEvent() {}
	ZST_EXPORT virtual void run(ZstSynchronisable * target) {};
	ZST_EXPORT void cast_run(ZstSynchronisable * target) override;
};

class ZstEntityBase;
class ZstEntityEvent : public ZstSynchronisableEvent {
public:
	ZST_EXPORT virtual ~ZstEntityEvent() {}
	ZST_EXPORT virtual void run(ZstEntityBase * target) {};
	ZST_EXPORT virtual void cast_run(ZstSynchronisable * target) override;
};

class ZstComponent;
class ZstComponentEvent : public ZstEntityEvent {
public:
	ZST_EXPORT virtual ~ZstComponentEvent() {}
	ZST_EXPORT virtual void run(ZstComponent * target) {};
	ZST_EXPORT virtual void cast_run(ZstSynchronisable * target) override;
};

class ZstComponentTypeEvent : public ZstComponentEvent {
public:
	ZST_EXPORT virtual ~ZstComponentTypeEvent() {}
	ZST_EXPORT virtual void run(ZstComponent * target) {};
	ZST_EXPORT virtual void cast_run(ZstSynchronisable * target) override;
};

class ZstCable;
class ZstCableEvent : public ZstSynchronisableEvent {
public:
	ZST_EXPORT virtual ~ZstCableEvent() {}
	ZST_EXPORT virtual void run(ZstCable * target) {};
	ZST_EXPORT virtual void cast_run(ZstSynchronisable * target) override;
};

class ZstPlug;
class ZstPlugEvent : public ZstEntityEvent {
public:
	ZST_EXPORT virtual ~ZstPlugEvent() {}
	ZST_EXPORT virtual void run(ZstPlug * target) {};
	ZST_EXPORT virtual void cast_run(ZstSynchronisable * target) override;
};

class ZstInputPlug;
class ZstInputPlugEvent : public ZstEntityEvent {
public:
	ZST_EXPORT virtual ~ZstInputPlugEvent() {}
	ZST_EXPORT virtual void run(ZstInputPlug * target) {};
	ZST_EXPORT virtual void cast_run(ZstSynchronisable * target) override;
};

class ZstPerformer;
class ZstPerformerEvent : public ZstComponentEvent {
public:
	ZST_EXPORT virtual ~ZstPerformerEvent() {}
	ZST_EXPORT virtual void run(ZstPerformer * target) {};
	ZST_EXPORT virtual void cast_run(ZstSynchronisable * target) override;
};
