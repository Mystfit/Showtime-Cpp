#include "ZstPerformer.h"
#include <vector>

using namespace std;

ZstPerformer::ZstPerformer(string name) {
	m_name = name;
}

string ZstPerformer::get_name()
{
	return m_name;
}

std::vector<ZstPlug*> ZstPerformer::get_plugs() {
	vector<ZstPlug*> plugs;

	for (map<string, vector<ZstPlug*>>::iterator instrumentIter = m_plugs.begin(); instrumentIter != m_plugs.end(); ++instrumentIter) {
		for (vector<ZstPlug*>::iterator plugIter = instrumentIter->second.begin(); plugIter != instrumentIter->second.end(); ++plugIter) {
			plugs.push_back(*plugIter);
		}
	}
	return plugs;
}

std::vector<ZstPlug*> ZstPerformer::get_instrument_plugs(std::string instrument) {
	return m_plugs[instrument];
}

void ZstPerformer::add_plug(ZstPlug * plug)
{
	m_plugs[plug->get_instrument()].push_back(plug);
}

void ZstPerformer::remove_plug(ZstPlug * plug)
{
	m_plugs[plug->get_instrument()].erase(std::remove(m_plugs[plug->get_instrument()].begin(), m_plugs[plug->get_instrument()].end(), plug), m_plugs[plug->get_instrument()].end());
}
