#include <string>
#include <sstream>
#include <showtime/schemas/messaging/graph_types_generated.h>
#include <boost/bimap.hpp>
#include <boost/assign/list_of.hpp>
#include <showtime/ZstLogging.h>
#include "ZstValue.h"

using namespace flatbuffers;

namespace showtime {

	

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


void ZstDynamicValue::copy(const ZstIValue* from)
{
	if (from->get_default_type() == get_default_type()) {
		copy_direct(from);
		return;
	}

	// We will need to convert the incoming value to a destination type
	switch (from->get_default_type()) {
	case ZstValueType::IntList:
		copy_convert_from_source(from->int_buffer(), from->size());
		break;
	case ZstValueType::FloatList:
		copy_convert_from_source(from->float_buffer(), from->size());
		break;
	case ZstValueType::StrList:
	{
		char** str_buffer = nullptr;
		from->string_buffer(&str_buffer);
		copy_convert_from_source(const_cast<const char**>(str_buffer), from->size());
		break;
	}
	case ZstValueType::ByteList:
		copy_convert_from_source(from->byte_buffer(), from->size());
		break;
	default:
		break;
	}
}

void ZstDynamicValue::copy_direct(const ZstIValue* from)
{
	switch (get_default_type())
	{
	case ZstValueType::IntList:
		assign(from->int_buffer(), from->size());
		break;
	case ZstValueType::FloatList:
		assign(from->float_buffer(), from->size());
		break;
	case ZstValueType::StrList:
	{
		char** data = nullptr;
		from->string_buffer(&data);
		assign_strings(const_cast<const char**>(data), from->size());
		break;
	}
	case ZstValueType::ByteList:
		assign(from->byte_buffer(), from->size());
		break;
	default:
		break;
	}
}

void ZstDynamicValue::copy_convert_from_source(const int* from, size_t size)
{
	switch (get_default_type())
	{
	case ZstValueType::FloatList:
		copy_from_buffer<ZstValueDetails::ZstValueFloatVisitor>(ZstValueType::IntList, from, size, m_float_buffer);
		break;
	case ZstValueType::StrList:
	{
		copy_from_buffer<ZstValueDetails::ZstValueStrVisitor>(ZstValueType::StrList, from, size, m_string_buffer);
		//copy_strings_from_buffer<ZstValueDetails::ZstValueStrVisitor, std::string>(ZstValueType::StrList, from, size, m_string_buffer);
		//copy_strings_from_buffer<ZstValueDetails::ZstValueStrVisitor>(ZstValueType::StrList, from, size, m_string_buffer);
		break;
	}
	case ZstValueType::ByteList:
		copy_from_buffer<ZstValueDetails::ZstValueByteVisitor>(ZstValueType::ByteList, from, size, m_byte_buffer);
		break;
	default:
		break;
	}
}

void ZstDynamicValue::copy_convert_from_source(const float* from, size_t size)
{
	switch (get_default_type())
	{
	case ZstValueType::IntList:
		copy_from_buffer<ZstValueDetails::ZstValueIntVisitor>(ZstValueType::IntList, from, size, m_int_buffer);
		break;
	case ZstValueType::StrList:
	{
		copy_from_buffer<ZstValueDetails::ZstValueStrVisitor>(ZstValueType::StrList, from, size, m_string_buffer);
		//copy_strings_from_buffer<ZstValueDetails::ZstValueStrVisitor, std::string>(ZstValueType::StrList, from, size, m_string_buffer);
		//copy_strings_from_buffer<ZstValueDetails::ZstValueStrVisitor>(ZstValueType::StrList, from, size, m_string_buffer);
		break;
	}
	case ZstValueType::ByteList:
		copy_from_buffer<ZstValueDetails::ZstValueByteVisitor>(ZstValueType::ByteList, from, size, m_byte_buffer);
		break;
	default:
		break;
	}
}

void ZstDynamicValue::copy_convert_from_source(const char** from, size_t size)
{
	switch (get_default_type())
	{
	case ZstValueType::IntList:
		m_int_buffer.resize(size);
		for (size_t idx = 0; idx < size; ++idx) {
			m_int_buffer[idx] = boost::apply_visitor(ZstValueDetails::ZstValueIntVisitor(), ZstValueVariant(from[idx]));
		}
		break;
	case ZstValueType::FloatList:
		m_float_buffer.resize(size);
		for (size_t idx = 0; idx < size; ++idx) {
			m_float_buffer[idx] = boost::apply_visitor(ZstValueDetails::ZstValueFloatVisitor(), ZstValueVariant(from[idx]));
		}
		break;
	case ZstValueType::ByteList:
		m_byte_buffer.resize(size);
		for (size_t idx = 0; idx < size; ++idx) {
			m_byte_buffer[idx] = boost::apply_visitor(ZstValueDetails::ZstValueByteVisitor(), ZstValueVariant(from[idx]));
		}
		break;
	default:
		break;
	}
}

void ZstDynamicValue::copy_convert_from_source(const uint8_t* from, size_t size)
{
	switch (get_default_type())
	{
	case ZstValueType::IntList:
		copy_from_buffer<ZstValueDetails::ZstValueIntVisitor>(ZstValueType::IntList, from, size, m_int_buffer);
		break;
	case ZstValueType::FloatList:
		copy_from_buffer<ZstValueDetails::ZstValueFloatVisitor>(ZstValueType::FloatList, from, size, m_float_buffer);
		break;
	case ZstValueType::StrList:
	{
		copy_from_buffer<ZstValueDetails::ZstValueStrVisitor>(ZstValueType::StrList, from, size, m_string_buffer);
		//copy_strings_from_buffer<ZstValueDetails::ZstValueStrVisitor, std::string>(ZstValueType::StrList, from, size, m_string_buffer);
		break;
	}
	default:
		break;
	}
}

void ZstDynamicValue::assign(const int* newData, size_t count)
{
	std::lock_guard<std::mutex> lock(m_lock);
	m_int_buffer.resize(count);
	std::copy(newData, newData + count, m_int_buffer.begin());
}

void ZstDynamicValue::assign(const float* newData, size_t count)
{
	std::lock_guard<std::mutex> lock(m_lock);
	m_float_buffer.resize(count);
	std::copy(newData, newData + count, m_float_buffer.begin());
}

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
        case ZstValueType::NONE:
        case ZstValueType::DynamicList:
        case ZstValueType::PlugHandshake:
        case ZstValueType::ZstValueType_Size:
            break;
    }
	return 0;
}

const int ZstDynamicValue::int_at(const size_t position) const
{
	if (m_default_type == ZstValueType::IntList) {
		return m_int_buffer.at(position);
	}
	return 0;
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
}

const uint8_t ZstDynamicValue::byte_at(const size_t position) const
{
	if (m_default_type == ZstValueType::ByteList) {
		return m_byte_buffer.at(position);
	}

	return 0;
}

const size_t ZstDynamicValue::size_at(const size_t position) const {    
	if (m_default_type == ZstValueType::StrList) {
		return m_string_buffer.at(position).size();
	}
    return 0;
}

const int* ZstDynamicValue::int_buffer() const
{
	return m_int_buffer.data();
}

const float* ZstDynamicValue::float_buffer() const
{
	return m_float_buffer.data();
}

const void ZstDynamicValue::string_buffer(char*** data) const
{
	std::vector<char*> cstrings;
	cstrings.reserve(m_string_buffer.size());
	for (size_t idx = 0; idx < m_string_buffer.size(); ++idx)
		cstrings.push_back(const_cast<char*>(m_string_buffer[idx].c_str()));

	memcpy(data, m_string_buffer.data(), m_string_buffer.size());
}

const uint8_t* ZstDynamicValue::byte_buffer() const
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
		dest = CreatePlugValue(buffer_builder, PlugValueData_FloatList, CreateFloatList(buffer_builder, vec_offset).Union());
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
		dest = CreatePlugValue(buffer_builder, PlugValueData_ByteList, CreateByteList(buffer_builder, vec_offset).Union());
		break;
	}
    case ZstValueType::DynamicList:
	case ZstValueType::PlugHandshake:
	case ZstValueType::NONE:
    case ZstValueType::ZstValueType_Size:
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

	ZstValueType incoming_data_type = value_type_lookup.right.at(buffer->values_type());

	if (incoming_data_type != m_default_type) {
		Log::net(Log::Level::debug, "Data types don't match. Conversion required.");
	}

	switch (buffer->values_type()) {
	case PlugValueData_IntList: { 
		copy_from_buffer<ZstValueDetails::ZstValueIntVisitor, int>(incoming_data_type, buffer->values_as_IntList()->val()->data(), buffer->values_as_IntList()->val()->size(), m_int_buffer);
		break;
	}
	case PlugValueData_FloatList: {
		copy_from_buffer<ZstValueDetails::ZstValueFloatVisitor, float>(incoming_data_type, buffer->values_as_FloatList()->val()->data(), buffer->values_as_FloatList()->val()->size(), m_float_buffer);
		break;
	}
	case PlugValueData_StrList: {
		copy_strings_from_buffer<ZstValueDetails::ZstValueStrVisitor, std::string>(incoming_data_type, buffer->values_as_StrList()->val(), buffer->values_as_StrList()->val()->size(), m_string_buffer);
		break;
	}
	case PlugValueData_ByteList: {
		copy_from_buffer<ZstValueDetails::ZstValueByteVisitor, uint8_t>(incoming_data_type, buffer->values_as_ByteList()->val()->data(), buffer->values_as_ByteList()->val()->size(), m_byte_buffer);
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
