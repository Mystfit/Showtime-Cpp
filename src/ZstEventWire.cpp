//
//  ZstEventWire.cpp
//  Showtime
//
//  Created by Byron Mallett on 1/07/17.
//
//

#include "ZstEventWire.h"

using namespace std;

ZstEventWire::ZstEventWire() : ZstEvent()
{
}

ZstEventWire::ZstEventWire(const ZstEventWire & copy) : ZstEvent(copy)
{
}

ZstEventWire::ZstEventWire(ZstEvent copy) : ZstEvent(copy){
    
}


ZstEventWire::ZstEventWire(ZstURI single, EventType event_type) : ZstEvent(single, event_type)
{
}

ZstEventWire::ZstEventWire(ZstURI first, ZstURI second, EventType event_type) : ZstEvent(first, second, event_type)
{
}
