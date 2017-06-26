#pragma once

#include <msgpack.hpp>
#include <string>
#include "ZstURI.h"

class ZstURIWire : public ZstURI {
public:
	ZstURIWire();
	ZstURIWire(const ZstURIWire &copy);
	ZstURIWire(ZstURI uri);

	// this function is appears to be a mere serializer
	template <typename Packer>
	void msgpack_pack(Packer& pk) const {
		// make array of two elements, by the number of class fields
		pk.pack_array(4);

		pk.pack_str(strlen(m_performer));
		pk.pack_str_body(m_performer, strlen(m_performer));

		pk.pack_str(strlen(m_instrument));
		pk.pack_str_body(m_instrument, strlen(m_instrument));

		pk.pack_str(strlen(m_name));
		pk.pack_str_body(m_name, strlen(m_name));

		pk.pack(m_direction);
	}

	// this function is looks like de-serializer, taking an msgpack object
	// and extracting data from it to the current class fields
	void msgpack_unpack(msgpack::object o) {
		// check if received structure is an array
		if (o.type != msgpack::type::ARRAY) { throw msgpack::type_error(); }

		const size_t size = o.via.array.size;

		// sanity check
		if (size < 4) return;

		// extract value of first array entry to a class field
		int perf_char_size = o.via.array.ptr[0].via.str.size;
		const char * perf_char = o.via.array.ptr[0].via.str.ptr;

		int ins_char_size = o.via.array.ptr[1].via.str.size;
		const char * ins_char = o.via.array.ptr[1].via.str.ptr;

		int name_char_size = o.via.array.ptr[2].via.str.size;
		const char * name_char = o.via.array.ptr[2].via.str.ptr;

		m_performer = new char[perf_char_size+1]();
		m_instrument = new char[ins_char_size+1]();
		m_name = new char[name_char_size+1]();

		strncpy(m_performer, perf_char, perf_char_size);
		strncpy(m_instrument, ins_char, ins_char_size);
		strncpy(m_name, name_char, name_char_size);

		msgpack::object d = o.via.array.ptr[3];
		m_direction = o.via.array.ptr[3].as<Direction>();
	}

	// destination of this function is unknown - i've never ran into scenary
	// what it was called. some explaination/documentation needed.
	template <typename MSGPACK_OBJECT>
	void msgpack_object(MSGPACK_OBJECT* o, msgpack::zone* z) const {

	}
};

MSGPACK_ADD_ENUM(ZstURI::Direction);
