#pragma once

#include <vector>
#include <iostream>
#include <map>
#include "ZstConstants.h"

#define MAX_MISSED_HEARTBEATS 3

class ZstEndpointRef {
public:
	ZstEndpointRef(std::string starting_uuid, std::string assigned_uuid, std::string endpoint);
	~ZstEndpointRef();
    
    //Endpoint identifing info
	std::string client_starting_uuid;
	std::string client_assigned_uuid;
	std::string endpoint_address;

    //Entities
	ZstComponentProxy * register_container(ZstEntityWire & entity);
	std::vector<ZstComponentProxy*> get_entity_proxies();
	ZstComponentProxy * get_entity_proxy_by_URI(const ZstURI & uri);
	void destroy_container(ZstComponentProxy * entity);
	
    //Heartbeat
	void set_heartbeat_active();
	void set_heartbeat_inactive();
	void clear_active_hearbeat();
	bool get_active_heartbeat();
	int get_missed_heartbeats();

    //Plugs
	ZstPlugRef * create_plug(const ZstURI & address, ZstPlugDirection direction);
	ZstPlugRef * get_plug_by_URI(const ZstURI & uri);
	std::vector<ZstPlugRef*> get_plug_refs();
	void destroy_plug(const ZstURI & plug);

private:
	std::unordered_map<ZstURI, ZstComponentProxy*> m_entities;
	std::unordered_map<ZstURI, ZstPlugRef*> m_plugs;
	bool m_heartbeat_active;
	int m_missed_heartbeats = 0;
};
