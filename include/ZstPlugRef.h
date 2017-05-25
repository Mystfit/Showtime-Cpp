#pragma once

#include "ZstExports.h"
#include "ZstPlug.h"

class ZstPlugRef {
public:
	ZstPlugRef(PlugAddress address);
	~ZstPlugRef();
	ZST_EXPORT PlugAddress get_address();
private:
	PlugAddress m_address;
};