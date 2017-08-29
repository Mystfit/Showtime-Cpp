#include "ZstValueWire.h"

ZstValueWire::ZstValueWire() : ZstValue()
{
}

ZstValueWire::ZstValueWire(const ZstValueWire & copy) : ZstValue(copy)
{
}

ZstValueWire::ZstValueWire(const ZstValue & copy) : ZstValue(copy)
{
}

ZstValueWire::ZstValueWire(ZstValueType t) : ZstValue(t)
{
}

void ZstValueWire::msgpack_unpack(msgpack::object o)
{
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
		else if (val.type == msgpack::type::FLOAT32) {
			m_values.push_back(val.as<float>());
		}
		else if (val.type == msgpack::type::STR) {
			std::string val_str = val.as<std::string>();
			m_values.push_back(val_str);
		}
	}
}
