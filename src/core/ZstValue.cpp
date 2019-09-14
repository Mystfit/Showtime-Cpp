#include <string>
#include <sstream>
#include <msgpack.hpp>
#include "ZstLogging.h"
#include <nlohmann/json.hpp>
#include "ZstValue.h"

namespace showtime {

ZstValue::ZstValue() : m_default_type(ValueType::ValueType_INT)
{
}

ZstValue::ZstValue(const ZstValue & other)
{
	m_default_type = other.m_default_type;
	m_values = other.m_values;
}

ZstValue::ZstValue(ValueType t) : m_default_type(t)
{
}

ZstValue::~ZstValue()
{
}

ValueType ZstValue::get_default_type() const
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
    int result = boost::apply_visitor(ZstValueDetails::ZstValueIntVisitor(), val);
	return result;
}

const float ZstValue::float_at(const size_t position) const
{
	auto val = m_values.at(position);
    float result = boost::apply_visitor(ZstValueDetails::ZstValueFloatVisitor(), val);
	return result;
}

void ZstValue::char_at(char * buf, const size_t position) const
{
	if (position < m_values.size() - 1)
		return;

	auto val = m_values.at(position);
    std::string val_s = boost::apply_visitor(ZstValueDetails::ZstValueStrVisitor(), val);
	memcpy(buf, val_s.c_str(), val_s.size());
}

const size_t ZstValue::size_at(const size_t position) const {
    auto val = m_values.at(position);
    
    if (m_default_type == ValueType::ValueType_INT) {
        return sizeof(int);
    }
    else if (m_default_type == ValueType::ValueType_FLOAT) {
        return sizeof(float);
    }
    else if (m_default_type == ValueType::ValueType_FLOAT) {
        std::string val_s = boost::apply_visitor(ZstValueDetails::ZstValueStrVisitor(), val);
        return val_s.size();
    } 
    return 0;
}

//void ZstValue::write_json(json & buffer) const
//{
//    buffer[get_value_field_name(ZstValueFields::DEFAULT_TYPE)] = get_default_type();
//    buffer[get_value_field_name(ZstValueFields::VALUES)] = json::array();
//    for (auto val : m_values) {
//        if (get_default_type() == ZstValueType::ZST_INT) {
//            buffer[get_value_field_name(ZstValueFields::VALUES)].push_back(boost::apply_visitor(ZstValueIntVisitor(), val));
//        } else if (get_default_type() == ZstValueType::ZST_FLOAT) {
//            buffer[get_value_field_name(ZstValueFields::VALUES)].push_back(boost::apply_visitor(ZstValueFloatVisitor(), val));
//        } else if (get_default_type() == ZstValueType::ZST_STRING) {
//            buffer[get_value_field_name(ZstValueFields::VALUES)].push_back(boost::apply_visitor(ZstValueStrVisitor(), val));
//        } else {
//            //Unknown value type
//        }
//    }
//}
//
//void ZstValue::read_json(const json & buffer)
//{
//    m_default_type = buffer[get_value_field_name(ZstValueFields::DEFAULT_TYPE)];
//    m_values.clear();
//
//    //Unpack values
//    for (auto v : buffer[get_value_field_name(ZstValueFields::VALUES)]) {
//        if (v.is_number_integer()) {
//            m_values.emplace_back(v.get<int>());
//        }
//        else if (v.is_number_float()) {
//            m_values.emplace_back(v.get<float>());
//        }
//        else if (v.is_string()) {
//            m_values.emplace_back(v.get<std::string>());
//        }
//    }
//}
//

void ZstValue::serialize(flatbuffers::Offset< std::vector<flatbuffers::Offset<ValueTypes> > > & serialized_offset, flatbuffers::FlatBufferBuilder & buffer_builder) const
{
    
    std::vector<flatbuffers::Offset<ValueTypes>> value_builder;
    for(auto value : m_values){
        switch(m_default_type){
            case ValueType_INT:
                break;
            case ValueType_FLOAT:
                break;
            case ValueType_STRING:
                break;
            case ValueType_NONE:
                break;
        }
        
    }
}

void ZstValue::deserialize(const std::vector<flatbuffers::Offset<ValueTypes> >* buffer)
{
    m_values.clear();
}
    
namespace ZstValueDetails {

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

}
}
