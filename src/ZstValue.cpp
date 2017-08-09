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

ZstValue::ZstValue(const ZstValue & other)
{
    m_values = other.m_values;
	m_default_type = other.m_default_type;
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

void ZstValue::append_char(const char * value)
{
	m_values.push_back(string(value));
}

void ZstValue::append_variant(msgpack::type::variant value)
{
    m_values.push_back(value);
}

const size_t ZstValue::size() const
{
	return m_values.size();
}

const size_t ZstValue::size_at(const size_t position) const
{
	auto val = m_values.at(position);
	if (val.is_uint64_t()) {
		return sizeof(val.as_uint64_t());
	}
	else if (val.is_int64_t()) {
		return sizeof(val.as_int64_t());
	}
	else if (val.is_double()) {
		return sizeof(val.as_double());
	}
	else if (val.is_string()) {
		return val.as_string().size();
	} 
	return 0;
}

const int ZstValue::int_at(const size_t position) const
{
	if (position < m_values.size() - 1)
		return NAN;

	auto val = m_values.at(position);
	//Return native value
	if (val.is_uint64_t()) {
		return val.as_uint64_t();
	}
	else if (val.is_int64_t()) {
		return val.as_int64_t();
	}
	else {
		//Convert value
		if (val.is_double()) {
			return int(val.as_double() + 0.5f);
		}
		else if (val.is_string()) {
			try {
				return stoi(val.as_string());
			}
			catch (std::invalid_argument) {}
		}
	}
	return 0;
}

const float ZstValue::float_at(const size_t position) const
{
	if (position < m_values.size() - 1)
		return NAN;

	auto val = m_values.at(position);
	//Return native value
	if (val.is_double()) {
		return (float)val.as_double();
	}
	else {
		//Convert value
		if (val.is_uint64_t()) {
			return float(val.as_uint64_t());
		}
		else if (val.is_int64_t()) {
			return float(val.as_int64_t());
		}
		else if (val.is_string()) {
			try {
				return stof(val.as_string());
			} catch (std::invalid_argument) {}
		}
	}
	return 0.0f;
}

void ZstValue::char_at(char * buf, const size_t position) const
{
	if (position < m_values.size() - 1)
		return;

	auto val = m_values.at(position);
	if (val.is_string()) {
		string s = val.as_string();
		memcpy(buf, s.c_str(), s.size());
	}
	else {
		if (val.is_uint64_t()) {
			memcpy(buf, std::to_string(val.as_uint64_t()).c_str(), sizeof(buf));
		}
		else if (val.is_int64_t()) {
			memcpy(buf, std::to_string(val.as_int64_t()).c_str(), sizeof(buf));
		}
		else if (val.is_double()) {
			memcpy(buf, std::to_string(val.as_double()).c_str(), sizeof(buf));
		}
	}
}

const msgpack::type::variant ZstValue::variant_at(const size_t position) const
{
    return m_values[position];
}

