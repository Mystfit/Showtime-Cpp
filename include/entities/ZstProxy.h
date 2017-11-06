//
//  ZstProxy.h
//  Showtime
//
//  Created by Byron Mallett on 6/11/17.
//
//

#pragma once

#include "ZstExports.h"
#include "entities/ZstEntityBase.h"

class ZstProxy : public ZstEntityBase {
public:
    ZST_EXPORT ZstProxy(const char * entity_type, const char * path);
    ZST_EXPORT ~ZstProxy();
    ZST_EXPORT virtual void init();
};
