#pragma once

#include "entities/ZstComponent.h"

class ZstProxyComponent : public ZstComponent {
public:
    ZstProxyComponent();
    ZstProxyComponent(const char * name);
    ZstProxyComponent(const char * name, ZstEntityBase * parent);
    ~ZstProxyComponent();
    virtual void init() override;
};
