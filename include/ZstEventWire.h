//
//  ZstEventWire.hpp
//  Showtime
//
//  Created by Byron Mallett on 1/07/17.
//
//

#pragma once
#include <iostream>
#include <msgpack.hpp>
#include "ZstURIWire.h"
#include "ZstEvent.h"

class ZstEventWire : public ZstEvent{
public:
    ZstEventWire();
    ZstEventWire(const ZstEventWire &copy);
    ZstEventWire(ZstEvent copy);
    ZstEventWire(ZstURI single, EventType event_type);
    ZstEventWire(ZstURI first, ZstURI second, EventType event_type);

    template <typename Packer>
    void msgpack_pack(Packer& pk) const {
        pk.pack_array(3);
        pk.pack(ZstURIWire(m_first));
        pk.pack(ZstURIWire(m_second));
        pk.pack(m_update_type);
    }
    
    void msgpack_unpack(msgpack::object o) {
        // check if received structure is an array
        if (o.type != msgpack::type::ARRAY) { throw msgpack::type_error(); }
        
        m_first = o.via.array.ptr[0].as<ZstURIWire>();
        m_second = o.via.array.ptr[1].as<ZstURIWire>();
        m_update_type = o.via.array.ptr[2].as<EventType>();
    }
};

MSGPACK_ADD_ENUM(ZstEvent::EventType);
