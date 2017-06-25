#pragma once

#include "ZstExports.h"
#include "ZstURI.h"

class ZstPlugRef {
public:
	ZstPlugRef(ZstURI uri);
	~ZstPlugRef();
	ZST_EXPORT ZstURI get_URI();
	ZST_EXPORT const std::vector<ZstURI> get_output_connections() const;
	ZST_EXPORT void add_output_connection(ZstURI uri);
	ZST_EXPORT void remove_output_connection(ZstURI uri);
private:
	ZstURI m_URI;
	std::vector<ZstURI> m_connections;
};