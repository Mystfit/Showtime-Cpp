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

ZstPlug * ZstPerformer::get_plug_by_name(std::string plug_name)
{
    vector<ZstPlug*> plugs = get_plugs();
    for (vector<ZstPlug*>::iterator plugIter = plugs.begin(); plugIter != plugs.end(); ++plugIter) {
        if((*plugIter)->get_URI().name() == plug_name){
            return (*plugIter);
        }
    }
    
    return NULL;
}

void ZstPerformer::add_plug(ZstPlug * plug)
{
	m_plugs[plug->get_URI().instrument()].push_back(plug);
}

void ZstPerformer::remove_plug(ZstPlug * plug)
{
	m_plugs[plug->get_URI().instrument()].erase(
		std::remove(
			m_plugs[plug->get_URI().instrument()].begin(), 
			m_plugs[plug->get_URI().instrument()].end(), plug
		), 
		m_plugs[plug->get_URI().instrument()].end()
	);
}
