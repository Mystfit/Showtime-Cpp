#pragma once

#include <msgpack.hpp>
#include "ZstURIWire.h"
#include "ZstExports.h"

class ZstURIWire;
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

	MSGPACK_DEFINE(m_first, m_second, m_update_type);

private:
	ZstURIWire m_first;
	ZstURIWire m_second;
	EventType m_update_type;
};

class ZstEventCallback {
public:
	ZST_EXPORT ZstEventCallback();
	virtual ~ZstEventCallback() { std::cout << "ZstEventCallback::~ZstEventCallback()" << std::endl; }
	virtual void run(ZstEvent e) { std::cout << "ZstEventCallback::run()" << std::endl; }
};

//Enums for MsgPack
MSGPACK_ADD_ENUM(ZstEvent::EventType);
