#pragma once

#include "entities/ZstComponent.h"

class ZstProxyComponent : public ZstComponent {
public:
    ZstProxyComponent();
    ZstProxyComponent(const char * name);
    ZstProxyComponent(const char * name, ZstEntityBase * parent);
    ~ZstProxyComponent();
    virtual void init() override;

	//Override activate so we don't accidentally re-register to the stage
	virtual void activate() override {};
};
