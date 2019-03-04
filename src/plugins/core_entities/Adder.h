#pragma once

#include "ZstExports.h"
#include "entities/ZstComponent.h"

#define ADDITION_FILTER_TYPE "addition"

class Adder : public ZstComponent {
public:
	ZST_ENTITY_EXPORT Adder(const char * name);
	ZST_ENTITY_EXPORT void compute(ZstInputPlug * plug) override;
	ZST_ENTITY_EXPORT ZstInputPlug* augend();
	ZST_ENTITY_EXPORT ZstInputPlug* addend();
	ZST_ENTITY_EXPORT ZstOutputPlug* sum();

private:
	ZstInputPlug * m_augend;
	ZstInputPlug * m_addend;
	ZstOutputPlug * m_sum;
};
