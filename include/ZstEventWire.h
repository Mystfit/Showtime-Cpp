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

    template <typename Packer>
	void msgpack_pack(Packer& pk) const {
        pk.pack_array(2);
        pk.pack(m_update_type);
        pk.pack_array(m_parameters.size());
		
        for(auto s : m_parameters){
            pk.pack_str(s.size());
            pk.pack_str_body(s.c_str(), s.size());
        }
	}

	void msgpack_unpack(msgpack::object o) {
		// check if received structure is an array
		if (o.type != msgpack::type::ARRAY) { throw msgpack::type_error(); }

        m_update_type = (ZstEvent::EventType)o.via.array.ptr[0].via.i64;
        
        int param_size = o.via.array.ptr[1].via.array.size;
        for(int i = 0; i < param_size; ++i){
            m_parameters.push_back(o.via.array.ptr[1].via.str.ptr);
        }
	}
};

MSGPACK_ADD_ENUM(ZstEvent::EventType);
