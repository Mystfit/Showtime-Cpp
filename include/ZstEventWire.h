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

		int sizeA = m_first.full_size() + 1;
		int sizeB = m_second.full_size() + 1;
		pk.pack_str(sizeA);
		pk.pack_str_body(m_first.path(), sizeA);
		pk.pack_str(sizeB);
		pk.pack_str_body(m_second.path(), sizeB);
		pk.pack(m_update_type);
	}

	void msgpack_unpack(msgpack::object o) {
		// check if received structure is an array
		if (o.type != msgpack::type::ARRAY) { throw msgpack::type_error(); }

		int sizeA = o.via.array.ptr[0].via.str.size;
		int sizeB = o.via.array.ptr[1].via.str.size;

		if(sizeA)
			m_first = ZstURI(o.via.array.ptr[0].via.str.ptr);
		if(sizeB)
			m_second = ZstURI(o.via.array.ptr[1].via.str.ptr);
		m_update_type = o.via.array.ptr[2].as<EventType>();
	}
};

MSGPACK_ADD_ENUM(ZstEvent::EventType);
