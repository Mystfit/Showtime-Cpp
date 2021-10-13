#include <string>
#include <sstream>
#include <showtime/schemas/messaging/graph_types_generated.h>
#include <boost/bimap.hpp>
#include <boost/assign/list_of.hpp>
#include <showtime/ZstLogging.h>
#include "ZstValue.h"

using namespace flatbuffers;

namespace showtime {

	namespace ZstValueDetails {
		class ZstValueIntVisitor : public boost::static_visitor<int>
		{
		public:
			int operator()(int i) const;
			int operator()(float f) const;
			int operator()(const std::string& str) const;
			int operator()(const uint8_t& b) const;
		};

		class ZstValueFloatVisitor : public boost::static_visitor<float>
		{
		public:
			float operator()(int i) const;
			float operator()(float f) const;
			float operator()(const std::string& str) const;
			float operator()(const uint8_t& b) const;
		};

		class ZstValueStrVisitor : public boost::static_visitor<std::string>
		{
		public:
			std::string operator()(int i) const;
			std::string operator()(float f) const;
			std::string operator()(const std::string& str) const;
			std::string operator()(const uint8_t& b) const;
		};

		class ZstValueByteVisitor : public boost::static_visitor<uint8_t>
		{
		public:
			uint8_t operator()(int i) const;
			uint8_t operator()(float f) const;
			uint8_t operator()(const std::string& str) const;
			uint8_t operator()(const uint8_t& b) const;
		};
	}

typedef boost::bimap<ZstValueType, PlugValueData> FlatbuffersEnityTypeMap;
static const FlatbuffersEnityTypeMap value_type_lookup = boost::assign::list_of< FlatbuffersEnityTypeMap::relation >
	(ZstValueType::NONE, PlugValueData_NONE)
	(ZstValueType::IntList, PlugValueData_IntList)
	(ZstValueType::FloatList, PlugValueData_FloatList)
	(ZstValueType::StrList, PlugValueData_StrList)
	(ZstValueType::ByteList, PlugValueData_ByteList)
	(ZstValueType::PlugHandshake, PlugValueData_PlugHandshake);

ZstValue::ZstValue() : m_default_type(ZstValueType::IntList)
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

ZstValue::ZstValue(const PlugValue* buffer)
{
	deserialize_partial(buffer);
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

void ZstValue::append_int(const int& value)
{
	std::lock_guard<std::mutex> lock(m_lock);
	m_values.push_back(value);
}

void ZstValue::append_float(const float& value)
{
	std::lock_guard<std::mutex> lock(m_lock);
	m_values.push_back(value);
}

void ZstValue::append_string(const char* value, const size_t size)
{
	std::lock_guard<std::mutex> lock(m_lock);
	m_values.push_back(std::string(value, size));
}

void ZstValue::append_byte(const uint8_t& value)
{
	std::lock_guard<std::mutex> lock(m_lock);
	m_values.push_back(value);
}

const size_t ZstValue::size() const
{
	return m_values.size();
}

const int ZstValue::int_at(const size_t position) const
{
	auto val = m_values.at(position);
	return boost::apply_visitor(ZstValueDetails::ZstValueIntVisitor(), val);
}

const float ZstValue::float_at(const size_t position) const
{
	auto val = m_values.at(position);
	return boost::apply_visitor(ZstValueDetails::ZstValueFloatVisitor(), val);
}

void ZstValue::string_at(char * buf, const size_t position) const
{
	auto val = m_values.at(position);
    std::string val_s = boost::apply_visitor(ZstValueDetails::ZstValueStrVisitor(), val);
	memcpy(buf, val_s.c_str(), val_s.size());
}

const uint8_t ZstValue::byte_at(const size_t position) const
{
	auto val = m_values.at(position);
	return boost::apply_visitor(ZstValueDetails::ZstValueByteVisitor(), val);
}

const size_t ZstValue::size_at(const size_t position) const {
    auto val = m_values.at(position);
    
    if (m_default_type == ZstValueType::IntList) {
        return sizeof(int);
    }
    else if (m_default_type == ZstValueType::FloatList) {
        return sizeof(float);
    }
    else if (m_default_type == ZstValueType::StrList) {
        std::string val_s = boost::apply_visitor(ZstValueDetails::ZstValueStrVisitor(), val);
        return val_s.size();
    } 
    return 0;
}
    
std::vector<int> ZstValue::as_int_vector() const
{
    std::vector<int> ivec;
    for(auto val : m_values){
		ivec.emplace_back(boost::apply_visitor(ZstValueDetails::ZstValueIntVisitor(), val));
    }
    return ivec;
}

std::vector<float> ZstValue::as_float_vector() const
{
    std::vector<float> fvec;
    for(auto val : m_values){
        fvec.emplace_back(boost::apply_visitor(ZstValueDetails::ZstValueFloatVisitor(), val));
    }
    return fvec;
}

std::vector<std::string> ZstValue::as_string_vector() const
{
    std::vector<std::string> svec;
    for(auto val : m_values){
        svec.emplace_back(boost::apply_visitor(ZstValueDetails::ZstValueStrVisitor(), val));
    }
    return svec;
}

std::vector<uint8_t> ZstValue::as_byte_vector() const
{
	std::vector<uint8_t> svec;
	for (auto val : m_values) {
		svec.emplace_back(boost::apply_visitor(ZstValueDetails::ZstValueByteVisitor(), val));
	}
	return svec;
}

uoffset_t ZstValue::serialize(flatbuffers::FlatBufferBuilder& buffer_builder) const
{
	Offset<PlugValue> dest;
	serialize_partial(dest, buffer_builder);
	return dest.o;
}

void ZstValue::serialize_partial(Offset<PlugValue>& dest, flatbuffers::FlatBufferBuilder& buffer_builder) const
{
	switch (m_default_type) {
	case ZstValueType::IntList:
		dest = CreatePlugValue(buffer_builder, PlugValueData_IntList, CreateIntList(buffer_builder, buffer_builder.CreateVector(as_int_vector())).Union());
		break;
	case ZstValueType::FloatList:
		dest = CreatePlugValue(buffer_builder, PlugValueData_FloatList, CreateFloatList(buffer_builder, buffer_builder.CreateVector(as_float_vector())).Union());
		break;
	case ZstValueType::StrList:
		dest = CreatePlugValue(buffer_builder, PlugValueData_StrList, CreateStrList(buffer_builder, buffer_builder.CreateVectorOfStrings(as_string_vector())).Union());
		break;
	case ZstValueType::ByteList:
		dest = CreatePlugValue(buffer_builder, PlugValueData_ByteList, CreateByteList(buffer_builder, buffer_builder.CreateVector(as_byte_vector())).Union());
		break;
	case ZstValueType::PlugHandshake:
		break;
	case ZstValueType::NONE:
		break;
	}
}

void ZstValue::deserialize(const PlugValue* buffer)
{
	deserialize_partial(buffer);
}

void ZstValue::deserialize_partial(const PlugValue* buffer)
{	
	if (!buffer) return;

	m_default_type = value_type_lookup.right.at(buffer->values_type());

	switch (buffer->values_type()) {
	case PlugValueData_IntList: {
		auto list = buffer->values_as_IntList();
		m_values.resize(list->val()->size());
		for (auto it = list->val()->begin(); it != list->val()->end(); ++it) {
			auto index = it - list->val()->begin();
			m_values[index] = *it;
		}
		break;
	}
	case PlugValueData_FloatList: {
		auto list = buffer->values_as_FloatList();
		m_values.resize(list->val()->size());
		for (auto it = list->val()->begin(); it != list->val()->end(); ++it) {
			auto index = it - list->val()->begin();
			m_values[index] = *it;
		}
		break;
	}
	case PlugValueData_StrList: {
		auto list = buffer->values_as_StrList();
		m_values.resize(list->val()->size());
		for (auto it = list->val()->begin(); it != list->val()->end(); ++it) {
			auto index = it - list->val()->begin();
			m_values[index] = it->str();
		}
		break;
	}
	case PlugValueData_ByteList: {
		auto list = buffer->values_as_ByteList();
		m_values.resize(list->val()->size());
		for (auto it = list->val()->begin(); it != list->val()->end(); ++it) {
			auto index = it - list->val()->begin();
			m_values[index] = *it;
		}
		break;
	}
	case PlugValueData_PlugHandshake:
		break;
	case PlugValueData_NONE:
		break;
	}
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

int ZstValueIntVisitor::operator()(const uint8_t& b) const
{
	return int(b);
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

float ZstValueFloatVisitor::operator()(const uint8_t& b) const
{
	return float(b);
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

std::string ZstValueStrVisitor::operator()(const uint8_t& b) const
{
	return std::to_string((char)b);
}

uint8_t ZstValueByteVisitor::operator()(int i) const
{
	return uint8_t(i);
}
uint8_t ZstValueByteVisitor::operator()(float f) const
{
	return uint8_t(f);
}
uint8_t ZstValueByteVisitor::operator()(const std::string& str) const
{
	return str.empty() ? 0 : (uint8_t)str[0];
}
uint8_t ZstValueByteVisitor::operator()(const uint8_t& b) const
{
	return b;
}

}
}
