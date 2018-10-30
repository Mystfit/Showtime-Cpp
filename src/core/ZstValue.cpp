#include <string>
#include <sstream>
#include <msgpack.hpp>
#include <ZstLogging.h>
#include <mpark/variant.hpp>
#include <nlohmann/json.hpp>
#include "ZstValue.h"

using namespace ZstValueDetails;

//Template instantiations
template class mpark::variant<int, float, std::string>;


ZstValue::ZstValue() : m_default_type(ZstValueType::ZST_NONE)
{
}

ZstValue::ZstValue(const ZstValue & other)
{
	m_default_type = other.m_default_type;
	m_values = other.m_values;
}

ZstValue::ZstValue(ZstValueType t) : m_default_type(t)
{
}

ZstValue::~ZstValue()
{
}

ZstValueType ZstValue::get_default_type() const
{
	return m_default_type;
}

void ZstValue::copy(const ZstValue & other)
{
	std::lock_guard<std::mutex> lock(m_lock);
	m_values = other.m_values;
}

void ZstValue::clear()
{
	std::lock_guard<std::mutex> lock(m_lock);
	m_values.clear();
}

void ZstValue::append_int(int value)
{
	std::lock_guard<std::mutex> lock(m_lock);
	m_values.push_back(value);
}

void ZstValue::append_float(float value)
{
	std::lock_guard<std::mutex> lock(m_lock);
	m_values.push_back(value);
}

void ZstValue::append_char(const char * value)
{
	std::lock_guard<std::mutex> lock(m_lock);
	m_values.push_back(std::string(value));
}

const size_t ZstValue::size() const
{
	return m_values.size();
}

const int ZstValue::int_at(const size_t position) const
{
	auto val = m_values.at(position);
	int result = visit(ZstValueIntVisitor(), val);
	return result;
}

const float ZstValue::float_at(const size_t position) const
{
	auto val = m_values.at(position);
	float result = visit(ZstValueFloatVisitor(), val);
	return result;
}

void ZstValue::char_at(char * buf, const size_t position) const
{
	if (position < m_values.size() - 1)
		return;

	auto val = m_values.at(position);
	std::string val_s = visit(ZstValueStrVisitor(), val);
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
        std::string val_s = visit(ZstValueStrVisitor(), val);
        return val_s.size();
    } 
    return 0;
}

void ZstValue::write(std::stringstream & buffer) const
{
	//Pack default type
	msgpack::pack(buffer, static_cast<unsigned int>(get_default_type()));

	//Pack values
	msgpack::pack(buffer, size());
	if (get_default_type() == ZstValueType::ZST_INT) {
		for (auto val : m_values) {
			msgpack::pack(buffer, visit(ZstValueIntVisitor(), val));
		}
	}
	else if (get_default_type() == ZstValueType::ZST_FLOAT) {
		for (auto val : m_values) {
			msgpack::pack(buffer, visit(ZstValueFloatVisitor(), val));
		}
	}
	else if (get_default_type() == ZstValueType::ZST_STRING) {
		for (auto val : m_values) {
			std::string s = visit(ZstValueStrVisitor(), val);
			msgpack::pack(buffer, s);
		}
	}
	else {

	}
}

void ZstValue::read(const char * buffer, size_t length, size_t & offset)
{
	//Payload is already in string format, have to unpack again
	auto handle = msgpack::unpack(buffer, length, offset);
	auto obj = handle.get();

	//Unpack default type
	m_default_type = static_cast<ZstValueType>(obj.via.i64);

	//Unpack num values
	handle = msgpack::unpack(buffer, length, offset);
	auto num_values = handle.get().via.i64;

	m_values.resize(num_values);

	//Unpack values
	for (int i = 0; i < num_values; ++i) {
		handle = msgpack::unpack(buffer, length, offset);
		auto val = handle.get();
		if (val.type == msgpack::type::NEGATIVE_INTEGER || val.type == msgpack::type::POSITIVE_INTEGER) {
			m_values[i] = static_cast<int>(val.via.i64);
		}
		else if (val.type == msgpack::type::FLOAT || val.type == msgpack::type::FLOAT32 || val.type == msgpack::type::FLOAT64 ){
			m_values[i] = static_cast<float>(val.via.f64);
		}
		else if (val.type == msgpack::type::STR) {
			m_values[i] = val.as<std::string>();
		} 
		else {
			//Unknown value type
		}
	}
}

void ZstValue::write_json(json & buffer) const
{
	buffer[get_value_field_name(ZstValueFields::DEFAULT_TYPE)] = get_default_type();
	buffer[get_value_field_name(ZstValueFields::VALUES)] = json::array();
	for (auto val : m_values) {
		if (get_default_type() == ZstValueType::ZST_INT) {
			buffer[get_value_field_name(ZstValueFields::VALUES)].push_back(visit(ZstValueIntVisitor(), val));
		} else if (get_default_type() == ZstValueType::ZST_FLOAT) {
			buffer[get_value_field_name(ZstValueFields::VALUES)].push_back(visit(ZstValueFloatVisitor(), val));
		} else if (get_default_type() == ZstValueType::ZST_STRING) {
			buffer[get_value_field_name(ZstValueFields::VALUES)].push_back(visit(ZstValueStrVisitor(), val));
		} else {
			//Unknown value type
		}
	}
}

void ZstValue::read_json(const json & buffer)
{
	m_default_type = buffer[get_value_field_name(ZstValueFields::DEFAULT_TYPE)];
	m_values.clear();

	//Unpack values
	for (auto v : buffer[get_value_field_name(ZstValueFields::VALUES)]) {
		if (v.is_number_integer()) {
			m_values.emplace_back(v.get<int>());
		}
		else if (v.is_number_float()) {
			m_values.emplace_back(v.get<float>());
		}
		else if (v.is_string()) {
			m_values.emplace_back(v.get<std::string>());
		}
	}
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
