#pragma once 

#include <set>
#include <string>

#include "ZstBundle.hpp"

typedef std::pair<std::string, std::string> ZstServerAddressPair;
typedef std::set<ZstServerAddressPair> ZstServerList;
typedef ZstBundle< ZstServerAddressPair > ZstServerBundle;
