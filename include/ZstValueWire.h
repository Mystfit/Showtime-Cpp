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
        
		if (m_default_type == ZstValueType::ZST_INT) {
            ZstValueIntVisitor visitor = ZstValueIntVisitor();
			for (auto val : m_values) {
				pk.pack_int(val.apply_visitor<ZstValueIntVisitor>(visitor));
			}
		}
		else if (m_default_type == ZstValueType::ZST_FLOAT) {
            ZstValueFloatVisitor visitor = ZstValueFloatVisitor();

			for (auto val : m_values) {
				pk.pack_float(val.apply_visitor<ZstValueFloatVisitor>(visitor));
			}
		}
		else if (m_default_type == ZstValueType::ZST_STRING) {
			for (auto val : m_values) {
                ZstValueStrVisitor visitor = ZstValueStrVisitor();
				std::string s = val.apply_visitor<ZstValueStrVisitor>(visitor);
				pk.pack_str(s.size());
				pk.pack_str_body(s.c_str(), s.size());
			}
		}
		else {

		}
	}

	void msgpack_unpack(msgpack::object o);
};
