#pragma once

#include "entities/ZstComponent.h"

#define ADDITION_FILTER_TYPE "addition"

class AddFilter : public ZstComponent {
public:
	ZST_EXPORT AddFilter(const char * name);
	ZST_EXPORT virtual void on_activated() override;
	ZST_EXPORT void compute(ZstInputPlug * plug) override;
	ZST_EXPORT ZstInputPlug* augend();
	ZST_EXPORT ZstInputPlug* addend();
	ZST_EXPORT ZstOutputPlug* sum();

private:
	ZstInputPlug * m_augend;
	ZstInputPlug * m_addend;
	ZstOutputPlug * m_sum;
};
