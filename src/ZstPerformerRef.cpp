#include "ZstPerformerRef.h"

using namespace std;

ZstPerformerRef::ZstPerformerRef(ZstURI uri) : m_URI(uri)
{
}

ZstPerformerRef::~ZstPerformerRef()
{
	vector<ZstPlugRef*> plugs = m_plugs;
	for (auto plug : plugs) {
		destroy_plug(plug);
	}
	m_plugs.clear();
}

ZstURI ZstPerformerRef::get_URI()
{
	return m_URI;
}

ZstPlugRef * ZstPerformerRef::create_plug(ZstURI address)
{
	//Check for existing plugs with this name
	auto it = find_if(m_plugs.begin(), m_plugs.end(), [&address](ZstPlugRef* plugRef) {return plugRef->get_URI() == address; });

	if (it != m_plugs.end()) {
		//Plug already exists!
		return NULL;
	}

	ZstPlugRef *plugRef = new ZstPlugRef(address);
	m_plugs.push_back(plugRef);
	return plugRef;
}

ZstPlugRef * ZstPerformerRef::get_plug_by_URI(ZstURI uri)
{
	auto it = find_if(m_plugs.begin(), m_plugs.end(), [&uri](ZstPlugRef* plugRef) {
		return plugRef->get_URI() == uri;
	});

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
