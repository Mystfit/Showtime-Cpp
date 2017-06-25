#pragma once

#include "ZstURI.h"
#include "ZstExports.h"
#include <msgpack.hpp>

class ZstURI;
class ZstEvent {
public:
	//Special message property enums
	enum EventType {
		CREATED,
		UPDATED,
		LEAVING,
		DESTROYED,
		CONNECTION_CREATED,
		CONNECTION_LEAVING,
		CONNECTION_DESTROYED,
		PLUG_HIT
	};

	ZstEvent();
	ZstEvent(ZstURI single, EventType event_type);
	ZstEvent(ZstURI first, ZstURI second, EventType event_type);
	ZST_EXPORT ~ZstEvent();

	ZST_EXPORT ZstURI get_first();
	ZST_EXPORT ZstURI get_second();
	ZST_EXPORT EventType get_update_type();

	MSGPACK_DEFINE(m_first, m_second, m_update_type);

private:
	ZstURI m_first;
	ZstURI m_second;
	EventType m_update_type;
};

//Enums for MsgPack
MSGPACK_ADD_ENUM(ZstEvent::EventType);
