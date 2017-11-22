#include "ZstEndpointRef.h"
#include "ZstEntityWire.h"
#include <exception>

using namespace std;

ZstEndpointRef::ZstEndpointRef(string starting_uuid, string assigned_uuid, string endpoint)
{
	client_starting_uuid = starting_uuid;
	client_assigned_uuid = assigned_uuid;
	endpoint_address = endpoint;
}

ZstEndpointRef::~ZstEndpointRef()
{
    for (auto entity_iter : m_entities) {
		delete entity_iter.second;
	}
	m_entities.clear();

	unordered_map<ZstURI, ZstPlugRef*> plugs = m_plugs;
	for (auto plug : plugs) {
		destroy_plug(plug.second->URI());
	}
	m_plugs.clear();
}

void ZstEndpointRef::set_heartbeat_active()
{
	m_heartbeat_active = true;
	m_missed_heartbeats = 0;
}

void ZstEndpointRef::clear_active_hearbeat() {
	m_heartbeat_active = false;
}

bool ZstEndpointRef::get_active_heartbeat()
{
	return m_heartbeat_active;
}

void ZstEndpointRef::set_heartbeat_inactive()
{
	m_missed_heartbeats++;
}

int ZstEndpointRef::get_missed_heartbeats()
{
	return m_missed_heartbeats;
}


ZstComponentProxy * ZstEndpointRef::register_entity(ZstEntityWire & entity)
{
	//Check for existing performers with this name
	if (m_entities.find(entity.URI()) != m_entities.end()) {
		return NULL;
	}
    
    ZstComponentProxy * proxy = new ZstComponentProxy(entity.entity_type(), entity.URI().path());
	m_entities[ZstURI(entity.URI())] = proxy;
	return proxy;
}

std::vector<ZstComponentProxy*> ZstEndpointRef::get_entity_proxies()
{
	vector<ZstComponentProxy*> entities;
	for (auto entity : m_entities) {
		entities.push_back(entity.second);
	}
	return entities;
}

ZstComponentProxy * ZstEndpointRef::get_entity_proxy_by_URI(const ZstURI & uri)
{
	ZstComponentProxy * result = NULL;
	auto it = m_entities.find(uri);
	if (it != m_entities.end()) {
		result = m_entities[uri];
	}
	return result;
}

void ZstEndpointRef::destroy_entity(ZstComponentProxy* entity)
{
	for (unordered_map<ZstURI, ZstComponentProxy*>::iterator entity_iter = m_entities.begin(); entity_iter != m_entities.end(); ++entity_iter)
	{
		if ((entity_iter)->second == entity)
		{
			m_entities.erase(entity_iter);
			break;
		}
	}

	delete entity;
}

// ----------------

ZstPlugRef * ZstEndpointRef::create_plug(const ZstURI & address, PlugDirection direction)
{
	ZstPlugRef * result = NULL;
	//Check for existing plugs with this name
	auto it = m_plugs.find(address);

	if (it == m_plugs.end()) {
		result = new ZstPlugRef(address, direction);
		m_plugs[address] = result;
	}
	else {
		cout << "ZST_STAGE: Plug already exists!" << endl;
	}

	return result;
}

ZstPlugRef * ZstEndpointRef::get_plug_by_URI(const ZstURI & uri)
{
	ZstPlugRef * result = NULL;
	if (m_plugs.empty())
		return NULL;

	auto plug_iter = m_plugs.find(uri);
	if (plug_iter != m_plugs.end()) {
		result = (*plug_iter).second;
	}
	return result;
}

std::vector<ZstPlugRef*> ZstEndpointRef::get_plug_refs()
{
	vector<ZstPlugRef*> plugs;
	for (auto plug : m_plugs) {
		plugs.push_back(plug.second);
	}
	return plugs;
}

void ZstEndpointRef::destroy_plug(const ZstURI & plug)
{
	ZstPlugRef * plug_to_delete = NULL;
	for (unordered_map<ZstURI, ZstPlugRef*>::iterator plug_iter = m_plugs.begin(); plug_iter != m_plugs.end(); ++plug_iter)
	{
		if (ZstURI::equal(plug_iter->second->URI(), plug))
		{
			plug_to_delete = plug_iter->second;
			m_plugs.erase(plug_iter);
			break;
		}
	}

	if(plug_to_delete)
		delete plug_to_delete;
}
