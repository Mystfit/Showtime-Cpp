#pragma once

#include <vector>
#include <iostream>
#include "ZstExports.h"
#include "ZstPlugRef.h"
#include "ZstPlug.h"

class ZstPerformerRef {
public:
	ZstPerformerRef(ZstURI uri);
	~ZstPerformerRef();

	ZST_EXPORT ZstURI get_URI();

	ZstPlugRef * create_plug(ZstURI address);
	ZST_EXPORT ZstPlugRef * get_plug_by_URI(ZstURI uri_str);
	ZST_EXPORT std::vector<ZstPlugRef*> & get_plug_refs();
	void destroy_plug(ZstPlugRef * plug);

	inline bool operator==(const ZstPerformerRef& other)
	{
		return (m_URI == other.m_URI);
	}
private:
	std::vector<ZstPlugRef*> m_plugs;
	ZstURI m_URI;
};
