#pragma once

#include "entities\ZstEntityBase.h"

#define ADDITION_FILTER_TYPE "addition"

class AddFilter : public ZstEntityBase {
public:
	AddFilter();

	void init() override;
	void compute(ZstInputPlug * plug) override;
private:
	ZstInputPlug * augend;
	ZstInputPlug * addend;
	ZstOutputPlug * sum;
};