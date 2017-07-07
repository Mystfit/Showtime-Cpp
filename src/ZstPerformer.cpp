#include "ZstPerformer.h"
#include "ZstPlug.h"

using namespace std;

ZstPerformer::ZstPerformer(string name) {
	m_name = name;
}

ZstPerformer::~ZstPerformer()
{
}

string ZstPerformer::get_name()
{
	return m_name;
}


ZstPlug * ZstPerformer::get_plug_by_URI(const ZstURI uri)
{
	string instrument = uri.instrument();
	vector<ZstPlug*> plugs = m_plugs[instrument];
    for (auto plug : plugs) {
        if(*(plug->get_URI()) == uri){
            return plug;
        }
    }
    
    return NULL;
}

void ZstPerformer::add_plug(ZstPlug * plug)
{
	m_plugs[plug->get_URI()->instrument()].push_back(plug);
}

void ZstPerformer::remove_plug(ZstPlug * plug)
{
	m_plugs[plug->get_URI()->instrument()].erase(
		std::remove(
			m_plugs[plug->get_URI()->instrument()].begin(),
			m_plugs[plug->get_URI()->instrument()].end(), plug
		), 
		m_plugs[plug->get_URI()->instrument()].end()
	);
}
