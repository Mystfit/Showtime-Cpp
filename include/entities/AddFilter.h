#pragma once

#include "entities\ZstFilter.h"
#include "ZstCallbacks.h"

#define ADDITION_FILTER_TYPE "addition"

class AddFilter : public ZstFilter {
public:
	ZST_EXPORT AddFilter(ZstEntityBase * parent);
	virtual void init() override;
	void compute(ZstInputPlug * plug) override;
	ZST_EXPORT ZstInputPlug* augend();
	ZST_EXPORT ZstInputPlug* addend();
	ZST_EXPORT ZstOutputPlug* sum();

private:
	ZstInputPlug * m_augend;
	ZstInputPlug * m_addend;
	ZstOutputPlug * m_sum;
};