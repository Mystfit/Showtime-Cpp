#pragma once

#include <vector>
#include <iostream>
#include "ZstExports.h"
#include "ZstPlugRef.h"
#include "ZstPlug.h"

class ZstPerformerRef {
public:
	ZstPerformerRef(std::string performer_name);
	~ZstPerformerRef();
	std::string name;

	ZstPlugRef * create_plug(ZstURI address);
	ZST_EXPORT ZstPlugRef * get_plug_by_URI(std::string uri_str);
	ZST_EXPORT std::vector<ZstPlugRef*> & get_plug_refs();
	void destroy_plug(ZstPlugRef * plug);

	inline bool operator==(const ZstPerformerRef& other)
	{
		return (name == other.name);
	}
private:
	std::vector<ZstPlugRef*> m_plugs;
};
