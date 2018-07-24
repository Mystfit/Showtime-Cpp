#pragma once

#include <ZstExports.h>
#include "ZstReaper.h"

class ZstModule {
public:
	virtual void init() = 0;
	virtual void destroy() = 0;
	
	ZST_EXPORT ZstReaper & reaper();

private:

	ZstReaper m_reaper;
};
