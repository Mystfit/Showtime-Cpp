#pragma once

#include "ZstExports.h"
#include "ZstURI.h"

class ZstEvent {
public:
	//Special message property enums
	enum EventType {
		NONE,
		PLUG_CREATED,
		PLUG_LEAVING,
		PLUG_DESTROYED,
		ENTITY_CREATED,
		ENTITY_LEAVING,
		ENTITY_DESTROYED,
		CABLE_CREATED,
		CABLE_LEAVING,
		CABLE_DESTROYED,
		ROUTE_CREATED,
		ROUTE_DESTROYED,
		PLUG_HIT
	};

	ZST_EXPORT ZstEvent();
	ZST_EXPORT ZstEvent(const ZstEvent & copy);
	ZST_EXPORT ZstEvent(ZstURI single, EventType event_type);
	ZST_EXPORT ZstEvent(ZstURI first, ZstURI second, EventType event_type);
	ZST_EXPORT ~ZstEvent();

	ZST_EXPORT ZstURI get_first() const;
	ZST_EXPORT ZstURI get_second() const;
	ZST_EXPORT EventType get_update_type();

	ZST_EXPORT bool operator==(const ZstEvent& other);
	ZST_EXPORT bool operator!=(const ZstEvent& other);

protected:
	ZstURI m_first;
	ZstURI m_second;
	EventType m_update_type;
};
