//
//  ZstEntityWire.cpp
//  Showtime
//
//  Created by Byron Mallett on 6/11/17.
//
//

#include "ZstEntityWire.h"

using namespace std;

ZstEntityWire::ZstEntityWire(){
    
}

ZstEntityWire::ZstEntityWire(const ZstEntityWire & copy) : ZstEntityBase(copy)
{
}

ZstEntityWire::ZstEntityWire(const ZstEntityBase & entity) : ZstEntityBase(entity)
{
}

ZstEntityWire::~ZstEntityWire()
{
}

