#pragma once

#include <iostream>
#include <unordered_map>
#include <sstream>
#include <ZstExports.h>
#include <ZstConstants.h>
#include <ZstURI.h>
#include <entities/ZstEntityFactory.h>
#include <entities/ZstContainer.h>

#define PERFORMER_TYPE "prf"

class ZstPerformer : public ZstContainer {
public:
	ZST_EXPORT ZstPerformer();
	ZST_EXPORT ZstPerformer(const char * name);
	ZST_EXPORT ZstPerformer(const ZstPerformer & other);
	ZST_EXPORT ~ZstPerformer();
	
    //Heartbeat
	ZST_EXPORT void set_heartbeat_active();
	ZST_EXPORT void set_heartbeat_inactive();
	ZST_EXPORT void clear_active_hearbeat();
	ZST_EXPORT bool get_active_heartbeat();
	ZST_EXPORT int get_missed_heartbeats();

	//Hierarchy
	ZST_EXPORT void add_child(ZstEntityBase * entity) override;
	ZST_EXPORT void remove_child(ZstEntityBase * entity) override;
	ZST_EXPORT void add_factory(ZstEntityFactory * factory);
	ZST_EXPORT void remove_factory(ZstEntityFactory * factory);
	ZST_EXPORT ZstEntityBundle & get_factories(ZstEntityBundle & bundle);
	ZST_EXPORT ZstEntityFactoryBundle & get_factories(ZstEntityFactoryBundle & bundle);

	
	//Serialisation
	ZST_EXPORT virtual void write(std::stringstream & buffer) const override;
	ZST_EXPORT virtual void read(const char * buffer, size_t length, size_t & offset) override;

private:
	//Heartbeat status
	bool m_heartbeat_active;
	int m_missed_heartbeats;

	//Creatables
	std::unordered_map<ZstURI, ZstEntityFactory*, ZstURIHash> m_factories;
};

typedef std::unordered_map<ZstURI, ZstPerformer*, ZstURIHash> ZstPerformerMap;
