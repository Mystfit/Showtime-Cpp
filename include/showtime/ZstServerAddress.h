#pragma once 

#include <set>
#include <string>
#include <showtime/ZstExports.h>
#include <showtime/ZstURI.h>
#include <showtime/ZstBundle.hpp>

namespace showtime {
	struct ZstServerAddress {
		std::string name;
		std::string address;
		ZST_EXPORT const char* c_name() const;
		ZST_EXPORT const char* c_address() const;
	};

	inline bool operator<(const ZstServerAddress& lhs, const ZstServerAddress& rhs) {
		return lhs.name < rhs.name;
	}

//class ZstServerAddress {
//public:    
//    ZST_EXPORT ZstServerAddress();
//    
//    ZST_EXPORT ZstServerAddress(const char* server_name, const char* server_address);
//    
//    ZST_EXPORT ZstServerAddress(const ZstServerAddress& other);
//
//    ZST_EXPORT ~ZstServerAddress();
//    
//    ZST_EXPORT ZstServerAddress(ZstServerAddress&& source) noexcept;
//    
//    ZST_EXPORT ZstServerAddress& operator=(const ZstServerAddress& rhs);
//    
//    ZST_EXPORT ZstServerAddress& operator=(ZstServerAddress&& rhs) noexcept;
//    
//    ZST_EXPORT bool operator<(const ZstServerAddress& rhs) const;
//
//    ZST_EXPORT bool operator==(const ZstServerAddress& rhs) const;
//
//    ZST_EXPORT const char* name();
//
//    ZST_EXPORT const char* address();
//
//private:
//    pstr m_name;
//    pstr m_address;
//};

typedef std::set<ZstServerAddress> ZstServerList;
typedef ZstBundle< ZstServerAddress > ZstServerAddressBundle;

}