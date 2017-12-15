#pragma once

#include <vector>
#include <iostream>
#include <map>
#include <sstream>
#include "ZstExports.h"
#include "ZstConstants.h"
#include "ZstURI.h"
#include "entities/ZstContainer.h"

#define MAX_MISSED_HEARTBEATS 3
#define PERFORMER_TYPE "performer"

class ZstPerformer : public ZstContainer {
public:
	ZST_EXPORT ZstPerformer();
	ZST_EXPORT ZstPerformer(const char * name, const char * address);
	ZST_EXPORT ~ZstPerformer();
	
    //Heartbeat
	ZST_EXPORT void set_heartbeat_active();
	ZST_EXPORT void set_heartbeat_inactive();
	ZST_EXPORT void clear_active_hearbeat();
	ZST_EXPORT bool get_active_heartbeat();
	ZST_EXPORT int get_missed_heartbeats();

	//Client properties
	void set_uuid(const char * uuid);
	ZST_EXPORT const char * uuid();
	ZST_EXPORT const char * address();

	//Creatables
	ZST_EXPORT int num_creatables();

	ZST_EXPORT virtual void write(std::stringstream & buffer) override;
	ZST_EXPORT virtual void read(const char * buffer, size_t length, size_t & offset) override;

private:
	//Heartbeat status
	bool m_heartbeat_active;
	int m_missed_heartbeats;
	
	//Endpoint identifing info
	std::string m_uuid;
	std::string m_address;

	//Creatables
	std::unordered_map<ZstURI, ZstEntityBase*> m_creatables;
};
