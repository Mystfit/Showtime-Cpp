#include <showtime/ZstServerAddress.h>

//namespace showtime {
//
//    ZstServerAddress::ZstServerAddress()
//    {
//        m_name = create_pstr("");
//        m_address = create_pstr("");
//    }
//
//    ZstServerAddress::ZstServerAddress(const char* server_name, const char* server_address)
//    {
//        m_name = create_pstr(server_name);
//        m_address = create_pstr(server_address);
//    }
//
//    ZstServerAddress::ZstServerAddress(const ZstServerAddress& other) :
//        m_name(other.m_name),
//        m_address(other.m_address)
//    {
//    }
//
//    ZstServerAddress::~ZstServerAddress() {
//        free(m_name.cstr);
//        free(m_address.cstr);
//    }
//
//    ZstServerAddress::ZstServerAddress(ZstServerAddress&& source) noexcept :
//        m_name(source.m_name),
//        m_address(source.m_address)
//    {
//        free(m_name.cstr);
//        free(m_address.cstr);
//        m_name = std::move(source.m_name);
//        m_address = std::move(source.m_address);
//
//        source.m_name = pstr();
//        source.m_address = pstr();
//    }
//
//    ZstServerAddress& ZstServerAddress::operator=(const ZstServerAddress& rhs)
//    {
//        m_name = create_pstr(rhs.m_name.cstr);
//        m_address = create_pstr(rhs.m_address.cstr);
//        return *this;
//    }
//
//    ZstServerAddress& ZstServerAddress::operator=(ZstServerAddress&& rhs) noexcept
//    {
//        free(m_name.cstr);
//        free(m_address.cstr);
//        m_name = std::move(rhs.m_name);
//        m_address = std::move(rhs.m_address);
//
//        rhs.m_name = pstr();
//        rhs.m_address = pstr();
//        return *this;
//    }
//
//    bool ZstServerAddress::operator<(const ZstServerAddress& rhs) const
//    {
//        return std::tie(m_name.cstr, m_address.cstr) < std::tie(rhs.m_name.cstr, rhs.m_address.cstr);
//    }
//
//    bool ZstServerAddress::operator==(const ZstServerAddress& rhs) const
//    {
//        return std::tie(m_name.cstr, m_address.cstr) == std::tie(rhs.m_name.cstr, rhs.m_address.cstr);
//    }
//
//    const char* ZstServerAddress::name() {
//        return m_name.cstr;
//    }
//
//    const char* ZstServerAddress::address() {
//        return m_address.cstr;
//    }
//}

const char* showtime::ZstServerAddress::c_name() const
{
    return name.c_str();
}

const char* showtime::ZstServerAddress::c_address() const
{
    return address.c_str();
}
