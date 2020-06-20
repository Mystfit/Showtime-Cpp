#pragma once

//A ZstValue is a generic value that represents some data 
//sent from one ZstPlug to another

#include <vector>
#include <iostream>
#include <showtime/ZstConstants.h>
#include <showtime/ZstExports.h>
#include <showtime/ZstSerialisable.h>
#include <showtime/ZstExports.h>
#include <showtime/ZstConstants.h>
#include <mutex>

#include <boost/variant.hpp>


//Typedefs

namespace showtime {

typedef boost::variant<int, float, std::string> ZstValueVariant;

class ZstValue : virtual ZstSerialisable<PlugValue, PlugValue> {
public:
	ZST_EXPORT ZstValue();
	ZST_EXPORT ZstValue(const ZstValue & other);
	ZST_EXPORT ZstValue(ZstValueType t);
	ZST_EXPORT ZstValue(const PlugValue* buffer);

	ZST_EXPORT virtual ~ZstValue();

	ZST_EXPORT ZstValueType get_default_type() const;
	
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
    
    ZST_EXPORT std::vector<int> as_int_vector() const;
    ZST_EXPORT std::vector<float> as_float_vector() const;
    ZST_EXPORT std::vector<std::string> as_string_vector() const;

	//Serialisation
    ZST_EXPORT flatbuffers::uoffset_t serialize(flatbuffers::FlatBufferBuilder & buffer_builder) const override;
	ZST_EXPORT void serialize_partial(flatbuffers::Offset<PlugValue>& dest, flatbuffers::FlatBufferBuilder& buffer_builder) const override;
	ZST_EXPORT void deserialize(const PlugValue* buffer) override;
	ZST_EXPORT void deserialize_partial(const PlugValue* buffer) override;


protected:
	std::vector<ZstValueVariant> m_values;
	ZstValueType m_default_type;

private:
	std::mutex m_lock;
};

}
