#pragma once

#include "entities\ZstFilter.h"

#define ADDITION_FILTER_TYPE "addition"

class AddFilter : public ZstFilter {
public:
	AddFilter();
	AddFilter(ZstURI parent);
	void init() override;
	void compute(ZstInputPlug * plug) override;
private:
	ZstInputPlug * augend;
	ZstInputPlug * addend;
	ZstOutputPlug * sum;
};