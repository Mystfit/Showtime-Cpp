#pragma once

#include "../core/ZstMessagePool.h"


class ZstCZMQMessagePool : public ZstMessagePool {
public:
	virtual void populate(int size) override;
	virtual ZstMessage * get() override;
};