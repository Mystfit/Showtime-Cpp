#pragma once

#include "ZstExports.h"
#include "ZstURI.h"

class ZstPlugRef {
public:
	ZstPlugRef(ZstURI uri);
	~ZstPlugRef();
	ZST_EXPORT ZstURI get_URI();
private:
	ZstURI m_URI;
};