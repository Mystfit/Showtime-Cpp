#include "ZstCableAddress.h"
#include <fmt/format.h>

namespace showtime {

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
    
ZstCableAddress::ZstCableAddress(const Cable* address)
{
    deserialize_imp(address);
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

flatbuffers::Offset<Cable> ZstCableAddress::serialize(CableBuilder & buffer_builder) const
{
    auto input_URI = buffer_builder.fbb_.CreateString(m_input_URI.path(), m_input_URI.full_size());
    auto output_URI = buffer_builder.fbb_.CreateString(m_output_URI.path(), m_output_URI.full_size());
    buffer_builder.add_input_URI(input_URI);
    buffer_builder.add_output_URI(output_URI);
    
    return buffer_builder.Finish();
}

void ZstCableAddress::deserialize(const Cable* buffer)
{
    deserialize_imp(buffer);
}

void ZstCableAddress::deserialize_imp(const Cable* buffer)
{
    m_output_URI = ZstURI(buffer->input_URI()->c_str(), buffer->input_URI()->size());
    m_input_URI = ZstURI(buffer->output_URI()->c_str(), buffer->output_URI()->size());
}

std::string ZstCableAddress::to_string() const
{
	return fmt::format("{} :~~> {}", get_output_URI().path(), get_input_URI().path());
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

}
