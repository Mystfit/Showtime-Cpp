#pragma once

#include <msgpack.hpp>
#include <string>
#include "ZstURI.h"

class ZstURIWire : public ZstURI {
public:
	ZstURIWire();
	ZstURIWire(const ZstURIWire &copy);
	ZstURIWire(ZstURI uri);

	template <typename Packer>
	void msgpack_pack(Packer& pk) const {
		pk.pack_array(4);
		pk.pack_str(strlen(m_performer));
		pk.pack_str_body(m_performer, strlen(m_performer));
		pk.pack_str(strlen(m_instrument));
		pk.pack_str_body(m_instrument, strlen(m_instrument));
		pk.pack_str(strlen(m_name));
		pk.pack_str_body(m_name, strlen(m_name));
		pk.pack(m_direction);
	}

	void msgpack_unpack(msgpack::object o) {
		// check if received structure is an array
		if (o.type != msgpack::type::ARRAY) { throw msgpack::type_error(); }

		const size_t size = o.via.array.size;

		// sanity check
		if (size < 4) return;

		int perf_char_size = o.via.array.ptr[0].via.str.size;
		const char * perf_char = o.via.array.ptr[0].via.str.ptr;

		int ins_char_size = o.via.array.ptr[1].via.str.size;
		const char * ins_char = o.via.array.ptr[1].via.str.ptr;

		int name_char_size = o.via.array.ptr[2].via.str.size;
		const char * name_char = o.via.array.ptr[2].via.str.ptr;

		assert(perf_char_size < 255 && ins_char_size < 255 && name_char_size < 255);

		memcpy(m_performer, perf_char, perf_char_size);
		memcpy(m_instrument, ins_char, ins_char_size);
		memcpy(m_name, name_char, name_char_size);
		m_direction = o.via.array.ptr[3].as<Direction>();
        build_combined_char();
	}
};

MSGPACK_ADD_ENUM(ZstURI::Direction);
