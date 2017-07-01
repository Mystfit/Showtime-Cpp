#pragma once

#include "ZstExports.h"
#include "ZstURI.h"

class ZstEvent {
public:
	//Special message property enums
	enum EventType {
		CREATED,
		UPDATED,
		LEAVING,
		DESTROYED,
		CABLE_CREATED,
		CABLE_LEAVING,
		CABLE_DESTROYED,
		PLUG_HIT
	};

	ZST_EXPORT ZstEvent();
	ZST_EXPORT ZstEvent(const ZstEvent & copy);
	ZST_EXPORT ZstEvent(ZstURI single, EventType event_type);
	ZST_EXPORT ZstEvent(ZstURI first, ZstURI second, EventType event_type);
	ZST_EXPORT ~ZstEvent();

	ZST_EXPORT const ZstURI & get_first() const;
	ZST_EXPORT const ZstURI & get_second() const;
	ZST_EXPORT EventType get_update_type();


protected:
	ZstURI m_first;
	ZstURI m_second;
	EventType m_update_type;
};

class ZstEventCallback {
public:
	ZST_EXPORT ZstEventCallback();
	virtual ~ZstEventCallback() { std::cout << "ZstEventCallback::~ZstEventCallback()" << std::endl; }
	virtual void run(ZstEvent e) { std::cout << "ZstEventCallback::run()" << std::endl; }
};
