#pragma once

#include <showtime/ZstConstants.h>
#include <boost/variant.hpp>

namespace showtime {

	typedef boost::variant<int, float, std::string, uint8_t> ZstValueVariant;



	namespace ZstValueDetails {
		class ZstValueIntVisitor : public boost::static_visitor<int>
		{
		public:
			int operator()(int i) const { return i; };
			int operator()(float f) const { return int(f); };
			int operator()(const std::string& str) const { return std::stoi(str); };
			int operator()(const uint8_t& b) const { return int(b); };
		};

		class ZstValueFloatVisitor : public boost::static_visitor<float>
		{
		public:
			float operator()(int i) const { return float(i); };
			float operator()(float f) const { return f; };
			float operator()(const std::string& str) const { return std::stof(str); };
			float operator()(const uint8_t& b) const { return float(b); };
		};

		class ZstValueStrVisitor : public boost::static_visitor<std::string>
		{
		public:
			std::string operator()(int i) const { return std::to_string(i); };
			std::string operator()(float f) const { return std::to_string(f); };
			std::string operator()(const std::string& str) const { return str; };
			std::string operator()(const uint8_t& b) const { return std::to_string((char)b); };
		};

		class ZstValueByteVisitor : public boost::static_visitor<uint8_t>
		{
		public:
			uint8_t operator()(int i) const { return uint8_t(i); };
			uint8_t operator()(float f) const { return uint8_t(f); };
			uint8_t operator()(const std::string& str) const { return str.empty() ? 0 : (uint8_t)str[0]; };
			uint8_t operator()(const uint8_t& b) const { return b; };
		};
	}

	//template<typename Visitor_T, typename Primitive_T, typename IncomingBuffer_T>
	//void copy_from_buffer(ZstValueType incoming_type, const IncomingBuffer_T* buffer, size_t size, std::vector<Primitive_T>& destination)
	//{
	//	destination.resize(size);
	//	if (incoming_type == m_default_type) {
	//		std::copy(buffer, buffer + size, destination.begin());
	//		return;
	//	}

	//	for (size_t idx = 0; idx < size; ++idx) {
	//		//Primitive_T val = static_cast<Primitive_T>();
	//		destination[idx] = boost::apply_visitor(Visitor_T(), ZstValueVariant(buffer[idx]));
	//	}
	//}

	//template<typename Visitor_T, typename Primitive_T, typename IncomingBuffer_T>
	//void copy_strings_from_buffer(ZstValueType incoming_type, const IncomingBuffer_T* buffer, size_t size, std::vector<Primitive_T>& destination)
	//{
	//	destination.resize(size);

	//	for (size_t idx = 0; idx < size; ++idx) {
	//		if (incoming_type == m_default_type)
	//			destination[idx] = buffer->GetAsString(idx)->str();
	//		else
	//			destination[idx] = boost::apply_visitor(Visitor_T(), ZstValueVariant(buffer->GetAsString(idx)->str()));
	//	}
	//}
}