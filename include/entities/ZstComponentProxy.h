//
//  ZstComponentProxy.h
//  Showtime
//
//  Created by Byron Mallett on 6/11/17.
//
//

#pragma once

#include <iostream>
#include "ZstExports.h"
#include "entities/ZstComponent.h"

class ZstComponentProxy : public ZstComponent {
public:
    ZST_EXPORT ZstComponentProxy(const char * entity_type, const char * path);
    ZST_EXPORT ~ZstComponentProxy();
    ZST_EXPORT virtual void destroy() override;
    ZST_EXPORT virtual void init() override;
    ZST_EXPORT virtual void create(const char * name, ZstEntityBase* parent) override;
    ZST_EXPORT virtual void compute(ZstInputPlug * plug) override {};
};
