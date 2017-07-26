#pragma once

#include <vector>
#include <iostream>
#include "ZstExports.h"
#include "ZstURI.h"

class ZstEntityRef {
public:
	ZstEntityRef(ZstURI uri, std::string entity_type);
	~ZstEntityRef();

	ZST_EXPORT ZstURI get_URI();
	ZST_EXPORT std::string get_entity_type();

	inline bool operator==(const ZstEntityRef& other)
	{
		return (m_URI == other.m_URI);
	}
private:
	ZstURI m_URI;
	std::string m_entity_type;
};
