#include "ZstPerformerRef.h"

using namespace std;

ZstPerformerRef::ZstPerformerRef(std::string performer_name)
{
	name = performer_name;
}

ZstPerformerRef::~ZstPerformerRef()
{
	for (vector<ZstPlugRef*>::iterator plug_iter = m_plugs.begin(); plug_iter != m_plugs.end(); ++plug_iter) {
		destroy_plug((*plug_iter));
		delete (*plug_iter);
	}
	m_plugs.clear();
}


ZstPlugRef * ZstPerformerRef::create_plug(ZstURI address)
{
	//Check for existing plugs with this name
	auto it = find_if(m_plugs.begin(), m_plugs.end(), [&address](ZstPlugRef* plugRef) {return plugRef->get_URI().name() == address.name(); });

	if (it != m_plugs.end()) {
		//Plug already exists!
		return NULL;
	}

	ZstPlugRef *plugRef = new ZstPlugRef(address);
	m_plugs.push_back(plugRef);
	return plugRef;
}

ZstPlugRef * ZstPerformerRef::get_plug_by_name(std::string plug_name)
{
	auto it = find_if(m_plugs.begin(), m_plugs.end(), [&plug_name](ZstPlugRef* plugRef) {return plugRef->get_URI().name() == plug_name; });

	if (it != m_plugs.end()) {
		return (*it);
	}
	return NULL;
}

std::vector<ZstPlugRef*> & ZstPerformerRef::get_plug_refs()
{
	return m_plugs;
}

void ZstPerformerRef::destroy_plug(ZstPlugRef * plug)
{
	for (vector<ZstPlugRef*>::iterator plug_iter = m_plugs.begin(); plug_iter != m_plugs.end(); ++plug_iter)
	{
		if ((*plug_iter) == plug)
		{
			m_plugs.erase(plug_iter);
			break;
		}
	}
	delete plug;
}
