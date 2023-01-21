#include <string>
#include <sstream>
#include <algorithm>
#include <showtime/schemas/messaging/graph_types_generated.h>
#include <boost/bimap.hpp>
#include <boost/assign/list_of.hpp>
#include <showtime/ZstLogging.h>
#include "ZstValueUtils.h"
#include "ZstFixedValue.h"

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

ZstFixedValue::ZstFixedValue()
{
}

ZstFixedValue::ZstFixedValue(const ZstFixedValue & other)
{
	m_default_type = other.m_default_type;
	m_fixed_size = other.m_fixed_size;

	if (m_fixed_size >= 0) {
		allocate_fixed_buffers();
	}
}

ZstFixedValue::ZstFixedValue(ZstValueType t, int fixed_size) : 
	m_default_type(t), 
	m_fixed_size(fixed_size)
{
	if (m_fixed_size >= 0) {
		allocate_fixed_buffers();
	}
}

ZstFixedValue::ZstFixedValue(const PlugValue* buffer)
{
	deserialize_partial(buffer);
}

ZstFixedValue::~ZstFixedValue()
{
}

ZstValueType ZstFixedValue::get_default_type() const
{
	return m_default_type;
}

void ZstFixedValue::clear()
{
	std::lock_guard<std::mutex> lock(m_lock);
	if(m_int_fixed_data)
		memset(m_int_fixed_data.get(), 0, sizeof m_int_fixed_data.get());
	if(m_float_fixed_data)
		memset(m_float_fixed_data.get(), 0, sizeof m_float_fixed_data.get());
	if(m_byte_fixed_data)
		memset(m_byte_fixed_data.get(), 0, sizeof m_byte_fixed_data.get());
	if(m_str_fixed_data)
		m_str_fixed_data->clear();
	m_append_cursor_idx = 0;
}


void ZstFixedValue::copy(const ZstIValue* from)
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

void ZstFixedValue::copy_direct(const ZstIValue* from)
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

void ZstFixedValue::copy_convert_from_source(const int* from, size_t size)
{
	switch (get_default_type())
	{
	case ZstValueType::FloatList:
		copy_from_buffer<ZstValueDetails::ZstValueFloatVisitor>(ZstValueType::IntList, from, size, m_float_fixed_data.get());
		break;
	case ZstValueType::StrList:
	{
		//copy_strings_from_buffer<ZstValueDetails::ZstValueStrVisitor, std::string>(ZstValueType::StrList, from, size, m_string_buffer);
		//copy_strings_from_buffer<ZstValueDetails::ZstValueStrVisitor>(ZstValueType::StrList, from, size, m_str_fixed_data.get());
		//copy_strings_from_buffer<ZstValueDetails::ZstValueStrVisitor>(ZstValueType::StrList, from, size, m_str_fixed_data.get());
		for (size_t idx = 0; idx < m_fixed_size; ++idx) {
			m_str_fixed_data.get()->insert(m_str_fixed_data->begin() + idx, boost::apply_visitor(ZstValueDetails::ZstValueStrVisitor(), ZstValueVariant(from[idx])));
		}
		break;
	}
	case ZstValueType::ByteList:
		copy_from_buffer<ZstValueDetails::ZstValueByteVisitor>(ZstValueType::ByteList, from, size, m_int_fixed_data.get());
		break;
	default:
		break;
	}
}

void ZstFixedValue::copy_convert_from_source(const float* from, size_t size)
{
	switch (get_default_type())
	{
	case ZstValueType::IntList:
		copy_from_buffer<ZstValueDetails::ZstValueIntVisitor>(ZstValueType::IntList, from, size, m_int_fixed_data.get());
		break;
	case ZstValueType::StrList:
	{
		//copy_from_buffer<ZstValueDetails::ZstValueStrVisitor>(ZstValueType::StrList, from, size, m_str_fixed_data.get());
		//copy_strings_from_buffer<ZstValueDetails::ZstValueStrVisitor, std::string>(ZstValueType::StrList, from, size, m_string_buffer);
		//copy_strings_from_buffer<ZstValueDetails::ZstValueStrVisitor>(ZstValueType::StrList, from, size, m_string_buffer);
		for (size_t idx = 0; idx < m_fixed_size; ++idx) {
			m_str_fixed_data.get()->insert(m_str_fixed_data->begin() + idx, boost::apply_visitor(ZstValueDetails::ZstValueStrVisitor(), ZstValueVariant(from[idx])));
		}
		break;
	}
	case ZstValueType::ByteList:
		copy_from_buffer<ZstValueDetails::ZstValueByteVisitor>(ZstValueType::ByteList, from, size, m_byte_fixed_data.get());
		break;
	default:
		break;
	}
}

void ZstFixedValue::copy_convert_from_source(const char** from, size_t size)
{
	switch (get_default_type())
	{
	case ZstValueType::IntList:
		for (size_t idx = 0; idx < size; ++idx) {
			m_int_fixed_data[idx] = boost::apply_visitor(ZstValueDetails::ZstValueIntVisitor(), ZstValueVariant(from[idx]));
		}
		break;
	case ZstValueType::FloatList:
		for (size_t idx = 0; idx < size; ++idx) {
			m_float_fixed_data[idx] = boost::apply_visitor(ZstValueDetails::ZstValueFloatVisitor(), ZstValueVariant(from[idx]));
		}
		break;
	case ZstValueType::ByteList:
		for (size_t idx = 0; idx < size; ++idx) {
			m_byte_fixed_data[idx] = boost::apply_visitor(ZstValueDetails::ZstValueByteVisitor(), ZstValueVariant(from[idx]));
		}
		break;
	default:
		break;
	}
}

void ZstFixedValue::copy_convert_from_source(const uint8_t* from, size_t size)
{
	switch (get_default_type())
	{
	case ZstValueType::IntList:
		copy_from_buffer<ZstValueDetails::ZstValueIntVisitor>(ZstValueType::IntList, from, size, m_int_fixed_data.get());
		break;
	case ZstValueType::FloatList:
		copy_from_buffer<ZstValueDetails::ZstValueFloatVisitor>(ZstValueType::FloatList, from, size, m_float_fixed_data.get());
		break;
	case ZstValueType::StrList:
		//copy_from_buffer<ZstValueDetails::ZstValueStrVisitor>(ZstValueType::StrList, from, size, m_str_fixed_data.get());
		for (size_t idx = 0; idx < m_fixed_size; ++idx) {
			m_str_fixed_data.get()->insert(m_str_fixed_data->begin() + idx, boost::apply_visitor(ZstValueDetails::ZstValueStrVisitor(), ZstValueVariant(from[idx])));
		}
		break;
	default:
		break;
	}
}

void ZstFixedValue::assign(const int* newData, size_t count)
{
	if (count <= m_fixed_size) {
		std::lock_guard<std::mutex> lock(m_lock);
		std::copy(newData, newData + count, &m_int_fixed_data[0]);
	}
}

void ZstFixedValue::assign(const float* newData, size_t count)
{
	if (count <= m_fixed_size) {
		std::lock_guard<std::mutex> lock(m_lock);
		std::copy(newData, newData + count, &m_float_fixed_data[0]);
	}
}

void ZstFixedValue::assign_strings(const char** newData, size_t count)
{
	if (count <= m_fixed_size) {
		std::lock_guard<std::mutex> lock(m_lock);
		std::copy(newData, newData + count, m_str_fixed_data->begin());
	}
}

void ZstFixedValue::assign(const uint8_t* newData, size_t count)
{
	if (count <= m_fixed_size) {
		std::lock_guard<std::mutex> lock(m_lock);
		std::copy(newData, newData + count, &m_byte_fixed_data[0]);
	}
}

void ZstFixedValue::take(int* newData, size_t count)
{
	if (newData) {
		m_int_fixed_data = std::unique_ptr<int[]>(newData);
		m_fixed_size = count;
	}
	else {
		m_int_fixed_data = nullptr;
		m_fixed_size = 0;
	}
}

void ZstFixedValue::take(float* newData, size_t count)
{
	if (newData) {
		m_float_fixed_data = std::unique_ptr<float[]>(newData);
		m_fixed_size = count;
	}
	else {
		m_float_fixed_data = nullptr;
		m_fixed_size = 0;
	}
}

void ZstFixedValue::take(char** newData, size_t count)
{
	if (newData) {
		assign_strings(const_cast<const char**>(newData), count);
		m_fixed_size = count;
	}
	else {
		m_str_fixed_data = nullptr;
		m_fixed_size = 0;
	}
}

void ZstFixedValue::take(uint8_t* newData, size_t count)
{
	if (newData) {
		m_byte_fixed_data = std::unique_ptr<uint8_t[]>(newData);
		m_fixed_size = count;
	}
	else {
		m_byte_fixed_data = nullptr;
		m_fixed_size = 0;
	}
}

void* ZstFixedValue::release()
{
	switch (m_default_type) {
	case ZstValueType::IntList:
		return m_int_fixed_data.release();
	case ZstValueType::FloatList:
		return m_float_fixed_data.release();
	case ZstValueType::StrList:
		return m_str_fixed_data.release();
	case ZstValueType::ByteList:
		return m_byte_fixed_data.release();
	}
	return nullptr;
}

void ZstFixedValue::append(const int& value)
{
	if (m_append_cursor_idx < m_fixed_size) {
		std::lock_guard<std::mutex> lock(m_lock);
		m_int_fixed_data[m_append_cursor_idx++] = value;
	}
}

void ZstFixedValue::append(const float& value)
{
	if (m_append_cursor_idx < m_fixed_size) {
		std::lock_guard<std::mutex> lock(m_lock);
		m_float_fixed_data[m_append_cursor_idx++] = value;
	}
}

void ZstFixedValue::append(const char* value, const size_t size)
{
	if (m_append_cursor_idx < m_fixed_size) {
		std::lock_guard<std::mutex> lock(m_lock);
		m_str_fixed_data->push_back(std::string(value, size));
	}
}

void ZstFixedValue::append(const uint8_t& value)
{
	if (m_append_cursor_idx < m_fixed_size) {
		std::lock_guard<std::mutex> lock(m_lock);
		m_byte_fixed_data[m_append_cursor_idx++] = value;
	}
}

void ZstFixedValue::append_int(const int& value)
{
	append(value);
}

void ZstFixedValue::append_float(const float& value)
{
	append(value);
}

void ZstFixedValue::append_string(const char* value, const size_t size)
{
	append(value, size);
}

void ZstFixedValue::append_byte(const uint8_t& value)
{
	append(value);
}

const size_t ZstFixedValue::size() const
{
	return m_fixed_size;
}

const int ZstFixedValue::int_at(const size_t position) const
{
	if (position < m_fixed_size && m_default_type == ZstValueType::IntList)
		return m_int_fixed_data[position];
	return 0;
}

const float ZstFixedValue::float_at(const size_t position) const
{
	if (position < m_fixed_size && m_default_type == ZstValueType::FloatList)
		return m_float_fixed_data[position];
	return 0.0f;
}

const char* ZstFixedValue::string_at(const size_t position, size_t& out_str_size) const
{
	if (position < m_fixed_size && m_default_type == ZstValueType::StrList) {
		out_str_size = m_str_fixed_data->at(position).size();
		return m_str_fixed_data->at(position).c_str();
	}
	return nullptr;
}

const uint8_t ZstFixedValue::byte_at(const size_t position) const
{
	if (position < m_fixed_size && m_default_type == ZstValueType::ByteList)
		return m_byte_fixed_data[position];
	return 0;
}

const size_t ZstFixedValue::size_at(const size_t position) const {
	if (m_default_type == ZstValueType::StrList) {
		return m_str_fixed_data->at(position).size();
	}
    return 0;
}

int ZstFixedValue::fixed_size() const
{
	return m_fixed_size;
}

const int* ZstFixedValue::int_buffer() const
{
	return m_int_fixed_data.get();
}

const float* ZstFixedValue::float_buffer() const
{
	return m_float_fixed_data.get();
}

const void ZstFixedValue::string_buffer(char*** data) const
{
	/*std::vector<char*> cstrings;
	cstrings.reserve(m_str_fixed_data->size());
	for (size_t idx = 0; idx < m_str_fixed_data->size(); ++idx)
		cstrings.push_back(const_cast<char*>(m_str_fixed_data->at(idx).c_str()));*/

	memcpy(data, m_str_fixed_data->data(), m_str_fixed_data->size());
}

const uint8_t* ZstFixedValue::byte_buffer() const
{
	return m_byte_fixed_data.get();
}

uoffset_t ZstFixedValue::serialize(flatbuffers::FlatBufferBuilder& buffer_builder) const
{
	Offset<PlugValue> dest;
	serialize_partial(dest, buffer_builder);
	return dest.o;
}

void ZstFixedValue::serialize_partial(Offset<PlugValue>& dest, flatbuffers::FlatBufferBuilder& buffer_builder) const
{

    switch (m_default_type) {
	case ZstValueType::IntList:
	{
		int* buf = nullptr;
		auto vec_offset = buffer_builder.CreateUninitializedVector<int>(m_fixed_size, &buf);
		memcpy(buf, m_int_fixed_data.get(), m_fixed_size * sizeof(int));
		dest = CreatePlugValue(buffer_builder, PlugValueData_IntList, CreateIntList(buffer_builder, vec_offset).Union());
		break;
	}
	case ZstValueType::FloatList:
	{
		float* buf = nullptr;
		auto vec_offset = buffer_builder.CreateUninitializedVector<float>(m_fixed_size, &buf);
		memcpy(buf, m_float_fixed_data.get(), m_fixed_size * sizeof(float));
		dest = CreatePlugValue(buffer_builder, PlugValueData_FloatList, CreateFloatList(buffer_builder, vec_offset).Union());
		break;
	}
	case ZstValueType::StrList:
	{
		dest = CreatePlugValue(buffer_builder, PlugValueData_StrList, CreateStrList(buffer_builder, buffer_builder.CreateVectorOfStrings(*m_str_fixed_data.get())).Union());
		break;
	}
	case ZstValueType::ByteList:
	{
		uint8_t* buf = nullptr;
		auto vec_offset = buffer_builder.CreateUninitializedVector<uint8_t>(m_fixed_size, &buf);
		memcpy(buf, m_byte_fixed_data.get(), m_fixed_size * sizeof(uint8_t));
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

void ZstFixedValue::deserialize(const PlugValue* buffer)
{
	deserialize_partial(buffer);
}

void ZstFixedValue::deserialize_partial(const PlugValue* buffer)
{	
	if (!buffer) return;

	ZstValueType incoming_data_type = value_type_lookup.right.at(buffer->values_type());

	if (incoming_data_type != m_default_type) {
		Log::net(Log::Level::debug, "Data types don't match. Conversion required.");
	}

	switch (buffer->values_type()) {
	case PlugValueData_IntList: { 
		memcpy(m_int_fixed_data.get(), buffer->values_as_IntList()->val()->data(), buffer->values_as_IntList()->val()->size());
		break;
	}
	case PlugValueData_FloatList: {
		memcpy(m_float_fixed_data.get(), buffer->values_as_FloatList()->val()->data(), buffer->values_as_FloatList()->val()->size());
		break;
	}
	case PlugValueData_StrList: {
		//copy_strings_from_buffer<ZstValueDetails::ZstValueStrVisitor, std::string>(incoming_data_type, buffer->values_as_StrList()->val(), buffer->values_as_StrList()->val()->size(), m_str_fixed_data.get());
		//destination.resize(size);
		auto strlist = buffer->values_as_StrList()->val();

		for (size_t idx = 0; idx < m_fixed_size; ++idx) {
			if (incoming_data_type == m_default_type)
				m_str_fixed_data->insert(m_str_fixed_data->begin() + idx, strlist->GetAsString(idx)->str());
			else
				m_str_fixed_data->insert(m_str_fixed_data->begin() + idx, boost::apply_visitor(ZstValueDetails::ZstValueStrVisitor(), ZstValueVariant(strlist->GetAsString(idx)->str())));
		}
		break;
	}
	case PlugValueData_ByteList: {
		memcpy(m_byte_fixed_data.get(), buffer->values_as_ByteList()->val()->data(), buffer->values_as_ByteList()->val()->size());
		break;
	}
	case PlugValueData_PlugHandshake:
		break;
	case PlugValueData_NONE:
		break;
	}
}

void ZstFixedValue::allocate_fixed_buffers()
{
	if (m_fixed_size > 0) {
		switch (m_default_type) {
		case ZstValueType::IntList:
			m_int_fixed_data = std::make_unique<int[]>(m_fixed_size);
			break;
		case ZstValueType::FloatList:
			m_float_fixed_data = std::make_unique<float[]>(m_fixed_size);
			break;
		case ZstValueType::ByteList:
			m_byte_fixed_data = std::make_unique<uint8_t[]>(m_fixed_size);
			break;
		case ZstValueType::StrList:
			m_str_fixed_data = std::make_unique<std::vector<std::string> >();
			m_str_fixed_data->reserve(m_fixed_size);
			break;
		default:
			break;
		}
	}
}

}
