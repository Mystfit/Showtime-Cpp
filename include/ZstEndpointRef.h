#pragma once

#include <vector>
#include <iostream>
#include "ZstPerformerRef.h"

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

	void destroy_performer(ZstPerformerRef* performer);

private:
	std::map<std::string, ZstPerformerRef*> m_performers;
};