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
	for (map<string, ZstPerformerRef*>::iterator perf_iter = m_performers.begin(); perf_iter != m_performers.end(); ++perf_iter) {
		delete perf_iter->second;
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

	ZstPerformerRef * performer = new ZstPerformerRef(name);
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
	for (map<string, ZstPerformerRef*>::iterator perf_iter = m_performers.begin(); perf_iter != m_performers.end(); ++perf_iter) {
		performers.push_back(perf_iter->second);
	}
	return performers;
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