#pragma once

#include <vector>
#include <iostream>
#include <map>
#include "ZstEntityRef.h"
#include "ZstPlugRef.h"

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
	ZstEntityRef * register_entity(std::string entity_type, const ZstURI & uri);
	std::vector<ZstEntityRef*> get_entity_refs();
	ZstEntityRef* get_entity_ref_by_URI(const ZstURI & uri);
	void destroy_entity(ZstEntityRef* entity);
    
    //Recipes
    void register_recipe(ZstCreatableRecipe recipe);
    void unregister_recipe(std::string recipe);
    const std::vector<ZstCreatableRecipe> & get_recipes();
	
    //Heartbeat
	void set_heartbeat_active();
	void set_heartbeat_inactive();
	void clear_active_hearbeat();
	bool get_active_heartbeat();
	int get_missed_heartbeats();

    //Plugs
	ZstPlugRef * create_plug(const ZstURI & address, PlugDirection direction);
	ZstPlugRef * get_plug_by_URI(const ZstURI & uri);
	std::vector<ZstPlugRef*> get_plug_refs();
	void destroy_plug(const ZstURI & plug);

private:
	std::unordered_map<ZstURI, ZstEntityRef*> m_entities;
	std::unordered_map<ZstURI, ZstPlugRef*> m_plugs;
    std::vector<ZstCreatableRecipe> m_recipes;
	bool m_heartbeat_active;
	int m_missed_heartbeats = 0;
};
