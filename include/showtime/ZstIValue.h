#pragma once

#include <showtime/ZstExports.h>
#include <showtime/ZstSerialisable.h>
#include <cstdint>

namespace showtime {
	class ZstIValue : public virtual ZstSerialisable<PlugValue, PlugValue> {
	public:
		ZST_EXPORT virtual ZstValueType get_default_type() const = 0;
		ZST_EXPORT virtual void clear() = 0;

		ZST_EXPORT virtual void copy(const ZstIValue* from) = 0;
		ZST_EXPORT virtual void copy_direct(const ZstIValue* from) = 0;
		ZST_EXPORT virtual void copy_convert_from_source(const int* from, size_t size) = 0;
		ZST_EXPORT virtual void copy_convert_from_source(const float* from, size_t size) = 0;
		ZST_EXPORT virtual void copy_convert_from_source(const char** from, size_t size) = 0;
		ZST_EXPORT virtual void copy_convert_from_source(const uint8_t* from, size_t size) = 0;

		ZST_EXPORT virtual void assign(const int* newData, size_t count) = 0;
		ZST_EXPORT virtual void assign(const float* newData, size_t count) = 0;
		//ZST_EXPORT virtual void assign(const char** newData, size_t count) = 0;
		ZST_EXPORT virtual void assign_strings(const char** newData, size_t count) = 0;
		ZST_EXPORT virtual void assign(const uint8_t* newData, size_t count) = 0;

		ZST_EXPORT virtual void take(int* newData, size_t count) = 0;
		ZST_EXPORT virtual void take(float* newData, size_t count) = 0;
		ZST_EXPORT virtual void take(char** newData, size_t count) = 0;
		ZST_EXPORT virtual void take(uint8_t* newData, size_t count) = 0;
		ZST_EXPORT virtual void* release() = 0;

		ZST_EXPORT virtual void append(const int& value) = 0;
		ZST_EXPORT virtual void append(const float& value) = 0;
		ZST_EXPORT virtual void append(const char* value, const size_t size) = 0;
		ZST_EXPORT virtual void append(const uint8_t& value) = 0;
		
		ZST_EXPORT virtual void append_int(const int& value) = 0;
		ZST_EXPORT virtual void append_float(const float& value) = 0;
		ZST_EXPORT virtual void append_string(const char* value, const size_t size) = 0;
		ZST_EXPORT virtual void append_byte(const uint8_t& value) = 0;

		ZST_EXPORT virtual const size_t size() const = 0;
		ZST_EXPORT virtual const size_t size_at(const size_t position) const = 0;
		ZST_EXPORT virtual int fixed_size() const = 0;

		ZST_EXPORT virtual const int* int_buffer() const = 0;
		ZST_EXPORT virtual const float* float_buffer() const = 0;
		ZST_EXPORT virtual const void string_buffer(char*** data) const = 0;
		ZST_EXPORT virtual const uint8_t* byte_buffer() const = 0;

		ZST_EXPORT virtual const int int_at(const size_t position) const = 0;
		ZST_EXPORT virtual const float float_at(const size_t position) const = 0;
		ZST_EXPORT virtual const char* string_at(const size_t position, size_t& out_str_size) const = 0;
		ZST_EXPORT virtual const uint8_t byte_at(const size_t position) const = 0;

		//Serialisation
		ZST_EXPORT flatbuffers::uoffset_t serialize(flatbuffers::FlatBufferBuilder& buffer_builder) const = 0;
		ZST_EXPORT void serialize_partial(flatbuffers::Offset<PlugValue>& dest, flatbuffers::FlatBufferBuilder& buffer_builder) const = 0;
		ZST_EXPORT void deserialize(const PlugValue* buffer) = 0;
		ZST_EXPORT void deserialize_partial(const PlugValue* buffer) = 0;
	};
}
