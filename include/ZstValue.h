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
	ZST_EXPORT void append_char(char * value);
	
	ZST_EXPORT size_t size();
	ZST_EXPORT int ZstValue::int_at(size_t position);
	ZST_EXPORT float ZstValue::float_at(size_t position);
	ZST_EXPORT const char* ZstValue::char_at(size_t position);

protected:
	std::vector<msgpack::type::variant> m_values;
	ZstValueType m_default_type;
};


//Variant visitors
//----------------
class ZstValue_as_int : public boost::static_visitor<int>
{
public:
	int operator()(int i) const;
	int operator()(float f) const;
	int operator()(char* s) const;
};

class ZstValue_as_float : public boost::static_visitor<float>
{
public:
	float operator()(int i) const;
	float operator()(float f) const;
	float operator()(char* s) const;
};

class ZstValue_as_char : public boost::static_visitor<const char*>
{
public:
	const char * operator()(int i) const;
	const char * operator()(float f) const;
	const char * operator()(char* s) const;
};
