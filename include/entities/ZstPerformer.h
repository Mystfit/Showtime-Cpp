#pragma once

#include <iostream>
#include <unordered_map>
#include <sstream>
#include <ZstExports.h>
#include <ZstConstants.h>
#include <ZstURI.h>
#include <entities/ZstContainer.h>

#define MAX_MISSED_HEARTBEATS 999999
#define PERFORMER_TYPE "prf"

typedef std::unordered_map<ZstURI, ZstPerformer*, ZstURIHash> ZstPerformerMap;

class ZstPerformer : public ZstContainer {
public:
	ZST_EXPORT ZstPerformer();
	ZST_EXPORT ZstPerformer(const char * name, const char * address);
	ZST_EXPORT ZstPerformer(const ZstPerformer & other);
	ZST_EXPORT ~ZstPerformer();
	
    //Heartbeat
	ZST_EXPORT void set_heartbeat_active();
	ZST_EXPORT void set_heartbeat_inactive();
	ZST_EXPORT void clear_active_hearbeat();
	ZST_EXPORT bool get_active_heartbeat();
	ZST_EXPORT int get_missed_heartbeats();

	//Client properties
	ZST_EXPORT const char * address();

	//Creatables
	ZST_EXPORT size_t num_creatables();

	ZST_EXPORT virtual void write(std::stringstream & buffer) override;
	ZST_EXPORT virtual void read(const char * buffer, size_t length, size_t & offset) override;

private:
	//Heartbeat status
	bool m_heartbeat_active;
	int m_missed_heartbeats;
	
	//Client identifing info
	std::string m_address;

	//Creatables
	ZstEntityMap m_creatables;
};
