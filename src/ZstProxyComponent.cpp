//
//  ZstProxyComponent.cpp
//  Showtime
//
//  Created by Byron Mallett on 7/09/17.
//
//

#include <stdio.h>
#include "entities/ZstProxyComponent.h"

ZstProxyComponent::ZstProxyComponent(const char * name) :
ZstComponent("PROXY", name)
{
    init();
}

ZstProxyComponent::ZstProxyComponent(const char * name, ZstEntityBase * parent) :
    ZstComponent("PROXY", name, parent)
{
    init();
}

ZstProxyComponent::~ZstProxyComponent(){
    
}

void ZstProxyComponent::init(){
    m_is_proxy = true;
}
