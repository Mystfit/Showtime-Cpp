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
		pk.pack_array(1);
		pk.pack_str(strlen(m_combined_path));
		pk.pack_str_body(m_combined_path, strlen(m_combined_path));
	}

	void msgpack_unpack(msgpack::object o) {
		// check if received structure is an array
		if (o.type != msgpack::type::ARRAY) { throw msgpack::type_error(); }

		int path_size = o.via.array.ptr[0].via.str.size;
		const char * path = o.via.array.ptr[0].via.str.ptr;

		memcpy(m_combined_path, path, path_size);
        build_split_path(m_combined_path);
	}
};
