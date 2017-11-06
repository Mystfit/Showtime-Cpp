//
//  ZstEntityWire.h
//  Showtime
//
//  Created by Byron Mallett on 6/11/17.
//
//
#pragma once
#include <msgpack.hpp>
#include "ZstURIWire.h"
#include "entities/ZstEntityBase.h"

class ZstEntityWire : public ZstEntityBase {
public:
    ZstEntityWire();
    ZstEntityWire(const ZstEntityWire &copy);
    ZstEntityWire(const ZstEntityBase & entity);
    ~ZstEntityWire();
    
    void init() override;
    
    template <typename Packer>
    void msgpack_pack(Packer& pk) const {
        pk.pack_array(2);
        pk.pack(m_uri);
        
        int size = strlen(m_entity_type)+1;
        pk.pack_str(size);
        pk.pack_str_body(m_entity_type, size);
    }
    
    void msgpack_unpack(msgpack::object o) {
        // check if received structure is an array
        if (o.type != msgpack::type::ARRAY) { throw msgpack::type_error(); }
        
        m_uri = o.via.array.ptr[0].as<ZstURIWire>();
        
        size_t size = o.via.array.ptr[1].via.str.size;
        m_entity_type = (char*)malloc(size+1);
        strncpy(m_entity_type, o.via.array.ptr[1].via.str.ptr, size);
    }
};
