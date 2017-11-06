#pragma once

#include "ZstURI.h"
#include <vector>
#include <string>

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
        TEMPLATE_CREATED,
        TEMPLATE_DESTROYED,
		ROUTE_CREATED,
		ROUTE_DESTROYED,
		PLUG_HIT
	};

	ZstEvent();
	ZstEvent(const ZstEvent & copy);
    ZstEvent(EventType event_type);
	~ZstEvent();

	EventType get_update_type();
    
    void add_parameter(std::string parameter);
    std::string get_parameter(size_t index);
protected:
	EventType m_update_type;
    std::vector<std::string> m_parameters;
};
