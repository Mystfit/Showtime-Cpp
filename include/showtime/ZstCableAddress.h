#pragma once

#include "ZstURI.h"
#include "ZstSerialisable.h"

class ZstCableAddress : public ZstSerialisable {
public:
    ZstCableAddress();
    ZstCableAddress(const ZstURI & input_URI, const ZstURI & output_URI);
    ZstCableAddress(const ZstCableAddress & other);
    ZstCableAddress(ZstCableAddress&& source);
    ZstCableAddress& operator=(const ZstCableAddress& other);
    ZstCableAddress& operator=(ZstCableAddress&& source);
    ~ZstCableAddress();
    
    //Comparison Operators
    
    ZST_EXPORT bool operator==(const ZstCableAddress & rhs) const;
    ZST_EXPORT bool operator!=(const ZstCableAddress & rhs) const;
    ZST_EXPORT bool operator<(const ZstCableAddress& rhs) const;
    
    //Accessors
    
    ZST_EXPORT const ZstURI & get_input_URI() const;
    ZST_EXPORT const ZstURI & get_output_URI() const;
    
    //Serialisation
    
    ZST_EXPORT void write_json(json & buffer) const override;
    ZST_EXPORT void read_json(const json & buffer) override;
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
