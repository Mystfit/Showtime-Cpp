//
//  ZstComponentProxy.cpp
//  Showtime
//
//  Created by Byron Mallett on 6/11/17.
//
//

#include "entities/ZstComponentProxy.h"

ZstComponentProxy::ZstComponentProxy(const char * entity_type, const char * path) : ZstComponent(entity_type, path)
{
}

ZstComponentProxy::~ZstComponentProxy()
{
}

void ZstComponentProxy::init()
{
}

void ZstComponentProxy::destroy()
{
}

void ZstComponentProxy::create(const char *name, ZstEntityBase *parent)
{
    //TODO: Implement proxy creation
    std::cout << "Create called on proxy" << std::endl;
}
