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
	m_default_type = other.m_default_type;
	m_values = other.m_values;
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

void ZstValue::append_char(const char * value)
{
	m_values.push_back(string(value));
}

const size_t ZstValue::size() const
{
	return m_values.size();
}

const int ZstValue::int_at(const size_t position) const
{
	if (position < m_values.size() - 1)
		return NAN;

	auto val = m_values.at(position);
    ZstValueIntVisitor visitor = ZstValueIntVisitor();
	int result = val.apply_visitor<ZstValueIntVisitor>(visitor);
	return result;
}

const float ZstValue::float_at(const size_t position) const
{
	if (position < m_values.size() - 1)
		return NAN;

	auto val = m_values.at(position);
    ZstValueFloatVisitor visitor = ZstValueFloatVisitor();
	float result = val.apply_visitor<ZstValueFloatVisitor>(visitor);
	return result;
}

void ZstValue::char_at(char * buf, const size_t position) const
{
	if (position < m_values.size() - 1)
		return;

	auto val = m_values.at(position);
    ZstValueStrVisitor visitor = ZstValueStrVisitor();
	std::string val_s = val.apply_visitor<ZstValueStrVisitor>(visitor);
	memcpy(buf, val_s.c_str(), val_s.size());
}

const size_t ZstValue::size_at(const size_t position) const {
    auto val = m_values.at(position);
    
    if (m_default_type == ZstValueType::ZST_INT) {
        return sizeof(int);
    }
    else if (m_default_type == ZstValueType::ZST_FLOAT) {
        return sizeof(float);
    }
    else if (m_default_type == ZstValueType::ZST_STRING) {
        ZstValueStrVisitor visitor = ZstValueStrVisitor();
        std::string val_s = val.apply_visitor<ZstValueStrVisitor>(visitor);
        return val_s.size();
    } 
    return 0;
}



// ----------------
// Variant visitors
// ----------------
int ZstValueIntVisitor::operator()(int i) const
{
	return i;
}

int ZstValueIntVisitor::operator()(float f) const
{
	return int(f);
}

int ZstValueIntVisitor::operator()(const std::string & str) const
{
	return std::stoi(str);
}

float ZstValueFloatVisitor::operator()(int i) const
{
	return float(i);
}

float ZstValueFloatVisitor::operator()(float f) const
{
	return f;
}

float ZstValueFloatVisitor::operator()(const std::string & str) const
{
	return std::stof(str);
}

std::string ZstValueStrVisitor::operator()(int i) const
{
	return std::to_string(i);
}

std::string ZstValueStrVisitor::operator()(float f) const
{
	return std::to_string(f);
}

std::string ZstValueStrVisitor::operator()(const std::string & str) const
{
	return str;
}
