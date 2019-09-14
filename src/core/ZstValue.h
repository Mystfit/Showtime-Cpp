#pragma once

//A ZstValue is a generic value that represents some data 
//sent from one ZstPlug to another

#include <vector>
#include <iostream>
#include <ZstConstants.h>
#include <ZstSerialisable.h>
#include <schemas/graph_types_generated.h>
#include "ZstExports.h"
#include <mutex>

#include <boost/bimap/bimap.hpp>
#include <boost/bimap/list_of.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/variant.hpp>


//Typedefs

namespace showtime {

typedef boost::variant<int, float, std::string> ZstValueVariant;


namespace ZstValueDetails {
	class ZstValueIntVisitor : public boost::static_visitor<int>
	{
	public:
		int operator()(int i) const;
		int operator()(float f) const;
		int operator()(const std::string & str) const;
	};

	class ZstValueFloatVisitor : public boost::static_visitor<float>
	{
	public:
		float operator()(int i) const;
		float operator()(float f) const;
		float operator()(const std::string & str) const;
	};

	class ZstValueStrVisitor : public boost::static_visitor<std::string>
	{
	public:
		std::string operator()(int i) const;
		std::string operator()(float f) const;
		std::string operator()(const std::string & str) const;
	};
}


class ZstValue : public ZstSerialisable< std::vector<flatbuffers::Offset<ValueTypes> > > {
public:
	ZST_EXPORT ZstValue();
	ZST_EXPORT ZstValue(const ZstValue & other);
	ZST_EXPORT ZstValue(ValueType t);
	ZST_EXPORT virtual ~ZstValue();

	ZST_EXPORT ValueType get_default_type() const;
	
	ZST_EXPORT void copy(const ZstValue & other);
	
	ZST_EXPORT void clear();
	ZST_EXPORT void append_int(int value);
	ZST_EXPORT void append_float(float value);
	ZST_EXPORT void append_char(const char * value);
	
	ZST_EXPORT const size_t size() const;
	ZST_EXPORT const int int_at(const size_t position) const;
	ZST_EXPORT const float float_at(const size_t position) const;
	ZST_EXPORT void char_at(char * buf, const size_t position) const;
	ZST_EXPORT const size_t size_at(const size_t position) const;

	//Serialisation
    ZST_EXPORT void serialize(flatbuffers::Offset< std::vector<flatbuffers::Offset<ValueTypes> > > & serialized_offset, flatbuffers::FlatBufferBuilder & buffer_builder) const override;
    ZST_EXPORT void deserialize(const std::vector<flatbuffers::Offset<ValueTypes> > * buffer) override;

protected:
	std::vector<ZstValueVariant> m_values;
	ValueType m_default_type;

private:
	std::mutex m_lock;
};

}
