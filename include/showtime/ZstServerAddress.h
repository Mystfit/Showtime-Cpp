#pragma once 

#include <set>
#include <string>

#include "ZstBundle.hpp"

namespace showtime {

class ZstServerAddress {
public:
    std::string name;
    std::string address;
    
    ZstServerAddress() : 
        name(""),
        address("")
    {
    }
    
    ZstServerAddress(const std::string & server_name, const std::string & server_address) :
        name(server_name),
        address(server_address)
    {
    }
    
    ZstServerAddress(const ZstServerAddress & other) :
        name(other.name),
        address(other.address)
    {
    }
    
    ZstServerAddress(ZstServerAddress && source) noexcept :
        name(source.name),
        address(source.address)
    {
        source.name.clear();
        source.address.clear();
    }
    
    ZstServerAddress & operator=(const ZstServerAddress & rhs)
    {
        name = rhs.name;
        address = rhs.address;
        return *this;
    }
    
    ZstServerAddress & operator=(ZstServerAddress && rhs)
    {
        name = std::move(rhs.name);
        address = std::move(rhs.address);
        return *this;
    }
    
    bool operator<(const ZstServerAddress & rhs) const
    {
        return std::tie(name, address) < std::tie(rhs.name, rhs.address);
    }

    bool operator==(const ZstServerAddress & rhs) const
    {
        return std::tie(name, address) == std::tie(rhs.name, rhs.address);
    }
};

typedef std::set<ZstServerAddress> ZstServerList;
typedef ZstBundle< ZstServerAddress > ZstServerAddressBundle;

}