#include "ZstCableAddress.h"

ZstCableAddress::ZstCableAddress() :
    m_input_URI(),
    m_output_URI()
{
}

ZstCableAddress::ZstCableAddress(const ZstURI & input_URI, const ZstURI & output_URI) :
    m_input_URI(input_URI),
    m_output_URI(output_URI)
{
}

ZstCableAddress::ZstCableAddress(const ZstCableAddress & other) :
    m_input_URI(other.m_input_URI),
    m_output_URI(other.m_output_URI)
{
}

ZstCableAddress::ZstCableAddress(ZstCableAddress&& source)
{
    m_input_URI = std::move(source.m_input_URI);
    m_output_URI = std::move(source.m_output_URI);
}

ZstCableAddress& ZstCableAddress::operator=(const ZstCableAddress& other)
{
    m_input_URI = other.m_input_URI;
    m_output_URI = other.m_output_URI;
    return *this;
}

ZstCableAddress& ZstCableAddress::operator=(ZstCableAddress&& source)
{
    m_input_URI = std::move(source.m_input_URI);
    m_output_URI = std::move(source.m_output_URI);
    return *this;
}

ZstCableAddress::~ZstCableAddress()
{
}

bool ZstCableAddress::operator==(const ZstCableAddress& rhs) const
{
    return std::tie(m_input_URI, m_output_URI) == std::tie(rhs.m_input_URI, rhs.m_output_URI);
}

bool ZstCableAddress::operator!=(const ZstCableAddress & rhs) const {
    return !(*this == rhs);
}

bool ZstCableAddress::operator<(const ZstCableAddress & rhs) const
{
    return std::tie( m_input_URI,  m_output_URI ) < std::tie(rhs.m_input_URI, rhs.m_output_URI);
}

const ZstURI & ZstCableAddress::get_input_URI() const
{
    return m_input_URI;
}

const ZstURI & ZstCableAddress::get_output_URI() const
{
    return m_output_URI;
}

void ZstCableAddress::write_json(json & buffer) const
{
    buffer["output_uri"] = m_output_URI.path();
    buffer["input_uri"] = m_input_URI.path();
}

void ZstCableAddress::read_json(const json & buffer)
{
    m_output_URI = ZstURI(buffer["output_uri"].get<std::string>().c_str(), buffer["output_uri"].get<std::string>().size());
    m_input_URI = ZstURI(buffer["input_uri"].get<std::string>().c_str(), buffer["input_uri"].get<std::string>().size());
}

size_t ZstCableAddressHash::operator()(ZstCableAddress const& k) const
{
    std::size_t h1 = ZstURIHash{}(k.get_output_URI());
    std::size_t h2 = ZstURIHash{}(k.get_input_URI());
    return h1 ^ (h2 << 1);
}

bool ZstCableAddressEq::operator()(const ZstCableAddress & lhs, const ZstCableAddress & rhs) const
{
    return lhs == rhs;
}
