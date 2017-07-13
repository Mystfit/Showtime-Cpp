#pragma once

#include "Showtime.h"
#include "ZstExports.h"
#include <boost\variant.hpp>
#include <msgpack.hpp>
#include <vector>
#include <iostream>

//A ZstValue is a generic value that represents some data 
//sent from one ZstPlug to another

class ZstValue {
public:
	ZstValue();
	ZstValue(ZstValueType t);
	~ZstValue();
	void init();

	ZST_EXPORT ZstValueType get_default_type();
	
	ZST_EXPORT void clear();
	ZST_EXPORT void append_int(int value);
	ZST_EXPORT void append_float(float value);
	ZST_EXPORT void append_char(const char * value);
	
	ZST_EXPORT const size_t size() const;
	ZST_EXPORT const size_t size_at(const size_t position) const;
	ZST_EXPORT const int ZstValue::int_at(const size_t position) const;
	ZST_EXPORT const float ZstValue::float_at(const size_t position) const;
	ZST_EXPORT void ZstValue::char_at(char * buf, const size_t position) const;

protected:
	std::vector<msgpack::type::variant> m_values;
	ZstValueType m_default_type;
};
