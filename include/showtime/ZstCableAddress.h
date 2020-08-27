#pragma once

#include <showtime/ZstURI.h>
#include <showtime/ZstSerialisable.h>

namespace showtime {

class ZST_CLASS_EXPORTED ZstCableAddress
#ifndef SWIG
    : public ZstSerialisable<Cable, CableData>
#endif
{
public:
    ZST_EXPORT ZstCableAddress();
    ZST_EXPORT ZstCableAddress(const ZstURI & input_URI, const ZstURI & output_URI);
    ZST_EXPORT ZstCableAddress(const Cable* buffer);
    ZST_EXPORT ZstCableAddress(const ZstCableAddress & other);
    ZST_EXPORT ZstCableAddress(ZstCableAddress&& source);
    ZST_EXPORT ZstCableAddress& operator=(const ZstCableAddress& other);
    ZST_EXPORT ZstCableAddress& operator=(ZstCableAddress&& source);
    ZST_EXPORT virtual ~ZstCableAddress();
    
    //Comparison Operators
    
    ZST_EXPORT bool operator==(const ZstCableAddress & rhs) const;
    ZST_EXPORT bool operator!=(const ZstCableAddress & rhs) const;
    ZST_EXPORT bool operator<(const ZstCableAddress& rhs) const;
    
    //Accessors
    
    ZST_EXPORT const ZstURI & get_input_URI() const;
    ZST_EXPORT const ZstURI & get_output_URI() const;
    
    //Serialisation
#ifndef SWIG
    ZST_EXPORT virtual flatbuffers::uoffset_t serialize(flatbuffers::FlatBufferBuilder & buffer_builder) const override;
    ZST_EXPORT virtual void serialize_partial(flatbuffers::Offset<CableData> & serialized_offset, flatbuffers::FlatBufferBuilder & buffer_builder) const override;
    ZST_EXPORT virtual void deserialize(const Cable* buffer) override;
    ZST_EXPORT virtual void deserialize_partial(const CableData* buffer) override;
#endif
	ZST_EXPORT std::string to_string() const;
    
private:
    ZstURI m_input_URI;
    ZstURI m_output_URI;
};

struct ZstCableAddressHash
{
    ZST_EXPORT size_t operator()(ZstCableAddress const& k) const;
};

struct ZstCableAddressEq {
    ZST_EXPORT bool operator()(ZstCableAddress const & lhs, ZstCableAddress const & rhs) const;
};

}
