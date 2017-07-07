#pragma once

#include <vector>
#include <iostream>
#include "ZstPerformerRef.h"

#define MAX_MISSED_HEARTBEATS 3

class ZstEndpointRef {
public:
	ZstEndpointRef(std::string starting_uuid, std::string assigned_uuid, std::string endpoint);
	~ZstEndpointRef();
	std::string client_starting_uuid;
	std::string client_assigned_uuid;
	std::string endpoint_address;

	ZstPerformerRef * create_performer(std::string name);
	ZST_EXPORT ZstPerformerRef * get_performer_by_name(std::string name);
	ZST_EXPORT std::vector<ZstPerformerRef*> get_performer_refs();
	
	void set_heartbeat_active();
	void set_heartbeat_inactive();
	void clear_active_hearbeat();
	bool get_active_heartbeat();
	int get_missed_heartbeats();

	void destroy_performer(ZstPerformerRef* performer);

private:
	std::map<std::string, ZstPerformerRef*> m_performers;

	bool m_heartbeat_active;
	int m_missed_heartbeats = 0;
};