#pragma once

#include "ZstExports.h"
#include "ZstURI.h"

class ZstPlugRef {
public:
	ZstPlugRef(ZstURI address);
	~ZstPlugRef();
	ZST_EXPORT ZstURI get_address();
private:
	ZstURI m_address;
};