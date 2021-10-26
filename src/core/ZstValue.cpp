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

ZstDynamicValue::ZstDynamicValue() : m_default_type(ZstValueType::IntList)
{
}

ZstDynamicValue::ZstDynamicValue(const ZstDynamicValue & other)
{
	m_default_type = other.m_default_type;
	m_int_buffer = other.m_int_buffer;
	m_float_buffer = other.m_float_buffer;
	m_string_buffer = other.m_string_buffer;
	m_byte_buffer = other.m_byte_buffer;
}

ZstDynamicValue::ZstDynamicValue(ZstValueType t) : m_default_type(t)
{
}

ZstDynamicValue::ZstDynamicValue(const PlugValue* buffer)
{
	deserialize_partial(buffer);
}

ZstDynamicValue::~ZstDynamicValue()
{
}

ZstValueType ZstDynamicValue::get_default_type() const
{
	return m_default_type;
}

void ZstDynamicValue::clear()
{
	std::lock_guard<std::mutex> lock(m_lock);
	m_int_buffer.clear();
	m_float_buffer.clear();
	m_string_buffer.clear();
	m_byte_buffer.clear();
}

void ZstDynamicValue::assign(const int* newData, size_t count)
{
	std::lock_guard<std::mutex> lock(m_lock);
	m_int_buffer.resize(count);
	//m_int_buffer.insert(m_int_buffer.begin(), &newData[0], &newData[count]);
	std::copy(newData, newData + count, m_int_buffer.begin());
}

void ZstDynamicValue::assign(const float* newData, size_t count)
{
	std::lock_guard<std::mutex> lock(m_lock);
	m_float_buffer.resize(count);
	//m_float_buffer.insert(m_float_buffer.begin(), &newData[0], &newData[count]);
	std::copy(newData, newData + count, m_float_buffer.begin());
}

//void ZstDynamicValue::assign(const char** newData, size_t count)
void ZstDynamicValue::assign_strings(const char** newData, size_t count)
{
	std::lock_guard<std::mutex> lock(m_lock);
	m_string_buffer.resize(count);
	m_string_buffer.insert(m_string_buffer.begin(), &newData[0], &newData[count]);
}

void ZstDynamicValue::assign(const uint8_t* newData, size_t count)
{
	std::lock_guard<std::mutex> lock(m_lock);
	m_byte_buffer.resize(count);
	//m_byte_buffer.insert(m_byte_buffer.begin(), &newData[0], &newData[count]);
	std::copy(newData, newData + count, m_byte_buffer.begin());
}

void ZstDynamicValue::append(const int& value)
{
	std::lock_guard<std::mutex> lock(m_lock);
	m_int_buffer.push_back(value);
}

void ZstDynamicValue::append(const float& value)
{
	std::lock_guard<std::mutex> lock(m_lock);
	m_float_buffer.push_back(value);
}

void ZstDynamicValue::append(const char* value, const size_t size)
{
	std::lock_guard<std::mutex> lock(m_lock);
	m_string_buffer.push_back(std::string(value, size));
}

void ZstDynamicValue::append(const uint8_t& value)
{
	std::lock_guard<std::mutex> lock(m_lock);
	m_byte_buffer.push_back(value);
}

void ZstDynamicValue::append_int(const int& value)
{
	append(value);
}

void ZstDynamicValue::append_float(const float& value)
{
	append(value);
}

void ZstDynamicValue::append_string(const char* value, const size_t size)
{
	append(value, size);
}

void ZstDynamicValue::append_byte(const uint8_t& value)
{
	append(value);
}

const size_t ZstDynamicValue::size() const
{
	switch (m_default_type) {
	case ZstValueType::IntList:
		return m_int_buffer.size();
		break;
	case ZstValueType::FloatList:
		return m_float_buffer.size();
		break;
	case ZstValueType::StrList:
		return m_string_buffer.size();
		break;
	case ZstValueType::ByteList:
		return m_byte_buffer.size();
		break;
	}
	return 0;
	//return m_dynamic_values.size();
}

const int ZstDynamicValue::int_at(const size_t position) const
{
	if (m_default_type == ZstValueType::IntList) {
		return m_int_buffer.at(position);
	}
	return 0;
	//auto val = m_dynamic_values.at(position);
	//return boost::apply_visitor(ZstValueDetails::ZstValueIntVisitor(), val);
}

const float ZstDynamicValue::float_at(const size_t position) const
{
	if(m_default_type == ZstValueType::FloatList)
		return m_float_buffer.at(position);
	
	return 0.0;
}

const char* ZstDynamicValue::string_at(const size_t position, size_t& out_str_size) const
{
	if (m_default_type == ZstValueType::StrList) {
		out_str_size = m_string_buffer.at(position).size();
		return m_string_buffer.at(position).c_str();
	}
		
	return nullptr;
	//auto val = m_dynamic_values.at(position);
	//std::string val_s = boost::apply_visitor(ZstValueDetails::ZstValueStrVisitor(), val);
	//memcpy(buf, val_s.c_str(), val_s.size());
}

const uint8_t ZstDynamicValue::byte_at(const size_t position) const
{
	if (m_default_type == ZstValueType::ByteList) {
		return m_byte_buffer.at(position);
	}

	return 0;

	/*auto val = m_dynamic_values.at(position);
	return boost::apply_visitor(ZstValueDetails::ZstValueByteVisitor(), val);*/
}

const size_t ZstDynamicValue::size_at(const size_t position) const {    
	if (m_default_type == ZstValueType::StrList) {
		return m_string_buffer.at(position).size();
	}
    return 0;
}

int* ZstDynamicValue::int_buffer()
{
	return m_int_buffer.data();
}

float* ZstDynamicValue::float_buffer()
{
	return m_float_buffer.data();
}

void ZstDynamicValue::string_buffer(char*** data)
{
	std::vector<char*> cstrings;
	cstrings.reserve(m_string_buffer.size());
	for (size_t idx = 0; idx < m_string_buffer.size(); ++idx)
		cstrings.push_back(const_cast<char*>(m_string_buffer[idx].c_str()));

	memcpy(data, m_string_buffer.data(), m_string_buffer.size());
}

uint8_t* ZstDynamicValue::byte_buffer()
{
	return m_byte_buffer.data();
}

uoffset_t ZstDynamicValue::serialize(flatbuffers::FlatBufferBuilder& buffer_builder) const
{
	Offset<PlugValue> dest;
	serialize_partial(dest, buffer_builder);
	return dest.o;
}

void ZstDynamicValue::serialize_partial(Offset<PlugValue>& dest, flatbuffers::FlatBufferBuilder& buffer_builder) const
{

	switch (m_default_type) {
	case ZstValueType::IntList:
	{
		int* buf = nullptr;
		auto vec_offset = buffer_builder.CreateUninitializedVector<int>(m_int_buffer.size(), &buf);
		memcpy(buf, m_int_buffer.data(), m_int_buffer.size() * sizeof(int));
		dest = CreatePlugValue(buffer_builder, PlugValueData_IntList, CreateIntList(buffer_builder, vec_offset).Union());
		break;
	}
	case ZstValueType::FloatList:
	{
		float* buf = nullptr;
		auto vec_offset = buffer_builder.CreateUninitializedVector<float>(m_float_buffer.size(), &buf);
		memcpy(buf, m_float_buffer.data(), m_float_buffer.size() * sizeof(float));
		dest = CreatePlugValue(buffer_builder, PlugValueData_IntList, CreateFloatList(buffer_builder, vec_offset).Union());
		//dest = CreatePlugValue(buffer_builder, PlugValueData_FloatList, CreateFloatList(buffer_builder, buffer_builder.CreateVector(as_float_vector())).Union());
		break;
	}
	case ZstValueType::StrList:
	{
		dest = CreatePlugValue(buffer_builder, PlugValueData_StrList, CreateStrList(buffer_builder, buffer_builder.CreateVectorOfStrings(m_string_buffer)).Union());
		break;
	}
	case ZstValueType::ByteList:
	{
		uint8_t* buf = nullptr;
		auto vec_offset = buffer_builder.CreateUninitializedVector<uint8_t>(m_byte_buffer.size(), &buf);
		memcpy(buf, m_byte_buffer.data(), m_byte_buffer.size() * sizeof(uint8_t));
		dest = CreatePlugValue(buffer_builder, PlugValueData_IntList, CreateByteList(buffer_builder, vec_offset).Union());
		//dest = CreatePlugValue(buffer_builder, PlugValueData_ByteList, CreateByteList(buffer_builder, buffer_builder.CreateVector(as_byte_vector())).Union());
		break;
	}
	case ZstValueType::PlugHandshake:
		break;
	case ZstValueType::NONE:
		break;
	}
}

void ZstDynamicValue::deserialize(const PlugValue* buffer)
{
	deserialize_partial(buffer);
}

void ZstDynamicValue::deserialize_partial(const PlugValue* buffer)
{	
	if (!buffer) return;

	m_default_type = value_type_lookup.right.at(buffer->values_type());

	switch (buffer->values_type()) {
	case PlugValueData_IntList: {
		auto list = buffer->values_as_IntList();
		m_int_buffer.resize(list->val()->size());
		std::copy(list->val()->data(), list->val()->data() + list->val()->size(), m_int_buffer.begin());
		//m_int_buffer.insert(m_int_buffer.begin(), &list->val()->data()[0], &list->val()->data()[list->val()->size()]);
		break;
	}
	case PlugValueData_FloatList: {
		auto list = buffer->values_as_FloatList();
		m_float_buffer.resize(list->val()->size());
		std::copy(list->val()->data(), list->val()->data() + list->val()->size(), m_float_buffer.begin());
		//m_float_buffer.insert(m_float_buffer.begin(), &list->val()->data()[0], &list->val()->data()[list->val()->size()]);
		break;
	}
	case PlugValueData_StrList: {
		auto list = buffer->values_as_StrList();
		m_string_buffer.resize(list->val()->size());
		for (flatbuffers::uoffset_t idx = 0; idx < list->val()->size(); ++idx) {
			auto s = list->val()->GetAsString(idx);
			m_string_buffer[idx] = std::string(s->data(), s->size());
		}
		break;
	}
	case PlugValueData_ByteList: {
		auto list = buffer->values_as_ByteList();
		m_byte_buffer.resize(list->val()->size());
		std::copy(list->val()->data(), list->val()->data() + list->val()->size(), m_byte_buffer.begin());
		//m_byte_buffer.insert(m_byte_buffer.begin(), list->val()->begin(), list->val()->end());
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
