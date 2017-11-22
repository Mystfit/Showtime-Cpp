#pragma once

#include "ZstExports.h"
#include "ZstURI.h"
#include "ZstPlug.h"

class ZstPlugRef {
public:
	ZstPlugRef(ZstURI uri, PlugDirection dir);
	~ZstPlugRef();
	ZST_EXPORT ZstURI URI();
    ZST_EXPORT PlugDirection get_direction();
private:
	ZstURI m_URI;
    PlugDirection m_direction;
};
