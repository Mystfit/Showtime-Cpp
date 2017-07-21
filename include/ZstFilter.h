#pragma once
#include "ZstEntityBase.h"

class ZstInputPlug;
class ZstFilter : public ZstEntityBase {
protected:
	virtual void compute(ZstInputPlug * plug) = 0;
};