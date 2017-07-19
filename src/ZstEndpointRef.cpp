#include "ZstEndpointRef.h"

using namespace std;

ZstEndpointRef::ZstEndpointRef(string starting_uuid, string assigned_uuid, string endpoint)
{
	client_starting_uuid = starting_uuid;
	client_assigned_uuid = assigned_uuid;
	endpoint_address = endpoint;
}

ZstEndpointRef::~ZstEndpointRef()
{
    for (auto perf_iter : m_performers) {
		delete perf_iter.second;
	}
	m_performers.clear();
}

ZstPerformerRef * ZstEndpointRef::create_performer(std::string name)
{
	//Check for existing performers with this name
	if (m_performers.find(name) != m_performers.end()) {
		//Already exists!
		return NULL;
	}
	ZstPerformerRef * performer = new ZstPerformerRef(ZstURI(name.c_str(), "", ""));
	m_performers[name] = performer;
	return performer;
}

ZstPerformerRef * ZstEndpointRef::get_performer_by_name(std::string name)
{
	if (m_performers.find(name) != m_performers.end()) {
		return m_performers[name];
	}
	return NULL;
}

std::vector<ZstPerformerRef*> ZstEndpointRef::get_performer_refs()
{
	vector<ZstPerformerRef*> performers;
    for (auto performer_iter : m_performers) {
		performers.push_back(performer_iter.second);
	}
	return performers;
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


void ZstEndpointRef::destroy_performer(ZstPerformerRef* performer)
{
	for (map<string, ZstPerformerRef*>::iterator perf_iter = m_performers.begin(); perf_iter != m_performers.end(); ++perf_iter)
	{
		if ((perf_iter->second) == performer)
		{
			m_performers.erase(perf_iter);
			break;
		}
	}
	delete performer;
}
