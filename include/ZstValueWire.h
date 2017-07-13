#pragma once

#include <iostream>
#include <msgpack.hpp>
#include "ZstValue.h"

class ZstValueWire : public ZstValue {
public:
	ZstValueWire();
	ZstValueWire(const ZstValueWire &copy);
	ZstValueWire(const ZstValue &copy);

	ZstValueWire(ZstValueType t);

	template <typename Packer>
	void msgpack_pack(Packer& pk) const {
		pk.pack_array(2);
		pk.pack_int((int)m_default_type);
		pk.pack_array(m_values.size());
		for (auto val : m_values) {
			pk.pack(val);
		}
	}

	void msgpack_unpack(msgpack::object o) {
		// check if received structure is an array
		if (o.type != msgpack::type::ARRAY) { throw msgpack::type_error(); }

		const size_t size = o.via.array.size;

		// sanity check
		if (size < 2) return;

		m_default_type = (ZstValueType)o.via.array.ptr[0].via.i64;

		for (auto val : o.via.array.ptr[1].via.array) {
			if (val.type == msgpack::type::NEGATIVE_INTEGER || val.type == msgpack::type::POSITIVE_INTEGER) {
				m_values.push_back(val.as<int>());
			}
			else if (val.type == msgpack::type::FLOAT) {
				m_values.push_back(val.as<float>());
			}
			else if (val.type == msgpack::type::STR) {
				std::string val_str = val.as<std::string>();
				m_values.push_back(val_str);
			}
		}
	}
};