#include "ZstValue.h"
#include <string>
#include <msgpack.hpp>
#include <sstream>
#include <ostream>

using namespace std;

ZstValue::ZstValue() : m_default_type(ZstValueType::ZST_NONE)
{
	init();
}

ZstValue::ZstValue(ZstValueType t) : m_default_type(t)
{
	init();
}

ZstValue::~ZstValue()
{
}

void ZstValue::init() {

}

ZstValueType ZstValue::get_default_type()
{
	return m_default_type;
}

void ZstValue::clear()
{
	m_values.clear();
}

void ZstValue::append_int(int value)
{
	m_values.push_back(value);
}

void ZstValue::append_float(float value)
{
	m_values.push_back(value);
}

void ZstValue::append_char(char * value)
{
	m_values.push_back(value);
}

size_t ZstValue::size()
{
	return m_values.size();
}

int ZstValue::int_at(size_t position)
{
	auto val = m_values.at(position);
	if (val.is_uint64_t()) {
		return val.as_uint64_t();
	}
	else if (val.is_int64_t()) {
		return val.as_int64_t();
	}
	return 0;
}


float ZstValue::float_at(size_t position)
{
	auto val = m_values.at(position);
	if (val.is_double()) {
		return (float)val.as_double();
	}
	return 0.0f;
}


const char* ZstValue::char_at(size_t position)
{
	auto val = m_values.at(position);
	if (val.is_string()) {
		return val.as_string().c_str();
	}
}


//ZstValue int conversion
//-----------------------
int ZstValue_as_int::operator()(int i) const
{
	return i;
}

int ZstValue_as_int::operator()(float f) const
{
	return (int)f;
}

int ZstValue_as_int::operator()(char* s) const
{
	return std::stoi(s);
}

//ZstValue float conversion
//-----------------------
float ZstValue_as_float::operator()(int i) const
{
	return (float)i;
}

float ZstValue_as_float::operator()(float f) const
{
	return f;
}

float ZstValue_as_float::operator()(char* s) const
{
	return std::stof(s);
}

//ZstValue char* conversion
//-------------------------
const char* ZstValue_as_char::operator()(int i) const
{
	return std::to_string(i).c_str();
}

const char* ZstValue_as_char::operator()(float f) const
{
	return std::to_string(f).c_str();
}

const char* ZstValue_as_char::operator()(char* s) const
{
	return s;
}
