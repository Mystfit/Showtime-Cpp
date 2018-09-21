#pragma once
#include <memory>
#include <ZstExports.h>

class ZstEventWakeup {
public:
	ZST_EXPORT virtual void wake() = 0;
	ZST_EXPORT virtual void wait() = 0;
};
