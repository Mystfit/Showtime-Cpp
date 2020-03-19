#include <string>
#include <sstream>
#include <schemas/graph_types_generated.h>
#include <boost/bimap.hpp>
#include <boost/assign/list_of.hpp>
#include "ZstLogging.h"
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
		};

		class ZstValueFloatVisitor : public boost::static_visitor<float>
		{
		public:
			float operator()(int i) const;
			float operator()(float f) const;
			float operator()(const std::string& str) const;
		};

		class ZstValueStrVisitor : public boost::static_visitor<std::string>
		{
		public:
			std::string operator()(int i) const;
			std::string operator()(float f) const;
			std::string operator()(const std::string& str) const;
		};
	}

typedef boost::bimap<ZstValueType, PlugValueData> FlatbuffersEnityTypeMap;
static const FlatbuffersEnityTypeMap value_type_lookup = boost::assign::list_of< FlatbuffersEnityTypeMap::relation >
	(ZstValueType::NONE, PlugValueData_NONE)
	(ZstValueType::IntList, PlugValueData_IntList)
	(ZstValueType::FloatList, PlugValueData_FloatList)
	(ZstValueType::StrList, PlugValueData_StrList)
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
