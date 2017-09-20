#pragma once

#include <msgpack.hpp>
#include "ZstURI.h"

class ZstURIWire : public ZstURI {
public:
	ZstURIWire();
	ZstURIWire(const ZstURIWire &copy);
	ZstURIWire(const ZstURI & uri);
	~ZstURIWire();

	template <typename Packer>
	void msgpack_pack(Packer& pk) const {
		pk.pack_array(1);
		int size = full_size()+1;
		pk.pack_str(size);
		pk.pack_str_body(path(), size);
	}

	void msgpack_unpack(msgpack::object o) {
		// check if received structure is an array
		if (o.type != msgpack::type::ARRAY) { throw msgpack::type_error(); }

		int path_size = o.via.array.ptr[0].via.str.size;
		const char * path = o.via.array.ptr[0].via.str.ptr;
		if (!path_size)
			path = "";

		original_path = create_pstr(path);
		segmented_path = create_pstr(path);
		split();
	}
};
