#pragma once

//A ZstDynamicValue is a generic value that represents some data 
//sent from one ZstPlug to another

#include <vector>
#include <iostream>
#include <showtime/ZstConstants.h>
#include <showtime/ZstExports.h>
#include <showtime/ZstConstants.h>
#include <showtime/ZstIValue.h>

#include <mutex>

#include <boost/variant.hpp>

//Typedefs

namespace showtime {

typedef boost::variant<int, float, std::string, uint8_t> ZstValueVariant;

class ZstDynamicValue : public ZstIValue {
public:
	ZST_EXPORT ZstDynamicValue();
	ZST_EXPORT ZstDynamicValue(const ZstDynamicValue & other);
	ZST_EXPORT ZstDynamicValue(ZstValueType t);
	ZST_EXPORT ZstDynamicValue(const PlugValue* buffer);

	ZST_EXPORT virtual ~ZstDynamicValue();

	ZST_EXPORT ZstValueType get_default_type() const override;
	ZST_EXPORT void clear() override;

	ZST_EXPORT void copy(const ZstIValue* from) override;
	ZST_EXPORT void assign(const int* newData, size_t count)  override;
	ZST_EXPORT void assign(const float* newData, size_t count)  override;
	ZST_EXPORT void assign_strings(const char** newData, size_t count) override;
	//ZST_EXPORT void assign(const char** newData, size_t count)  override;
	ZST_EXPORT void assign(const uint8_t* newData, size_t count)  override;

	ZST_EXPORT void append(const int& value) override;
	ZST_EXPORT void append(const float& value) override;
	ZST_EXPORT void append(const char* value, const size_t size) override;
	ZST_EXPORT void append(const uint8_t& value) override;
	
	// Dynamic appends
	ZST_EXPORT void append_int(const int& value) override;
	ZST_EXPORT void append_float(const float& value) override;
	ZST_EXPORT void append_string(const char * value, const size_t size) override;
	ZST_EXPORT void append_byte(const uint8_t& value) override;

	ZST_EXPORT const size_t size() const override;	
	ZST_EXPORT const size_t size_at(const size_t position) const override;

	ZST_EXPORT const int* int_buffer() const override;
	ZST_EXPORT const float* float_buffer() const override;
	ZST_EXPORT const void string_buffer(char*** data) const override;
	ZST_EXPORT const uint8_t* byte_buffer() const override;

	ZST_EXPORT const int int_at(const size_t position) const override;
	ZST_EXPORT const float float_at(const size_t position) const override;
	ZST_EXPORT const char* string_at(const size_t position, size_t& out_str_size) const override;
	ZST_EXPORT const uint8_t byte_at(const size_t position) const override;

	//Serialisation
    ZST_EXPORT flatbuffers::uoffset_t serialize(flatbuffers::FlatBufferBuilder & buffer_builder) const override;
	ZST_EXPORT void serialize_partial(flatbuffers::Offset<PlugValue>& dest, flatbuffers::FlatBufferBuilder& buffer_builder) const override;
	ZST_EXPORT void deserialize(const PlugValue* buffer) override;
	ZST_EXPORT void deserialize_partial(const PlugValue* buffer) override;


protected:
	//std::vector<ZstValueVariant> m_dynamic_values;
	std::vector<int> m_int_buffer;
	std::vector<float> m_float_buffer;
	std::vector<std::string> m_string_buffer;
	std::vector<uint8_t> m_byte_buffer;

	ZstValueType m_default_type;

private:
	std::mutex m_lock;
};

}