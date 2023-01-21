#pragma once

//A ZstDynamicValue is a generic value that represents some data 
//sent from one ZstPlug to another

#include <vector>
#include <span>
#include <iostream>
#include <memory>
#include <mutex>

#include <showtime/ZstConstants.h>
#include <showtime/ZstExports.h>
#include <showtime/ZstConstants.h>
#include <showtime/ZstIValue.h>


//Typedefs

namespace showtime {

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
	ZST_EXPORT void copy_direct(const ZstIValue* from) override;
	ZST_EXPORT void copy_convert_from_source(const int* from, size_t size) override;
	ZST_EXPORT void copy_convert_from_source(const float* from, size_t size) override;
	ZST_EXPORT void copy_convert_from_source(const char** from, size_t size) override;
	ZST_EXPORT void copy_convert_from_source(const uint8_t* from, size_t size) override;

	ZST_EXPORT void assign(const int* newData, size_t count)  override;
	ZST_EXPORT void assign(const float* newData, size_t count)  override;
	ZST_EXPORT void assign_strings(const char** newData, size_t count) override;
	//ZST_EXPORT void assign(const char** newData, size_t count)  override;
	ZST_EXPORT void assign(const uint8_t* newData, size_t count)  override;

	ZST_EXPORT virtual void take(int* newData, size_t count) override;
	ZST_EXPORT virtual void take(float* newData, size_t count) override;
	ZST_EXPORT virtual void take(char** newData, size_t count) override;
	ZST_EXPORT virtual void take(uint8_t* newData, size_t count) override;
	ZST_EXPORT virtual void* release() override;


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
	ZST_EXPORT virtual int fixed_size() const override;

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

private:
	template<typename Visitor_T, typename Primitive_T, typename IncomingBuffer_T>
	void copy_from_buffer(ZstValueType incoming_type, const IncomingBuffer_T* buffer, size_t size, std::vector<Primitive_T>& destination)
	{
		std::lock_guard<std::mutex> lock(m_lock);
		destination.resize(size);

		if (incoming_type == m_default_type) {
			std::copy(buffer, buffer + size, destination.begin());
			return;
		}

		for (size_t idx = 0; idx < size; ++idx) {
			destination[idx] = boost::apply_visitor(Visitor_T(), ZstValueVariant(buffer[idx]));
		}
	}

	template<typename Visitor_T, typename Primitive_T, typename IncomingBuffer_T>
	void copy_strings_from_buffer(ZstValueType incoming_type, const IncomingBuffer_T* buffer, size_t size, std::vector<Primitive_T>& destination)
	{
		destination.resize(size);

		for (size_t idx = 0; idx < size; ++idx) {
			if (incoming_type == m_default_type)
				destination[idx] = buffer->GetAsString(idx)->str();
			else
				destination[idx] = boost::apply_visitor(Visitor_T(), ZstValueVariant(buffer->GetAsString(idx)->str()));
		}
	}


protected:
	ZstValueType m_default_type = ZstValueType::IntList;

	std::vector<int> m_int_buffer;
	std::vector<float> m_float_buffer;
	std::vector<std::string> m_string_buffer;
	std::vector<uint8_t> m_byte_buffer;

private:
	std::mutex m_lock;
};

}