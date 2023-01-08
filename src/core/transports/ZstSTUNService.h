#pragma once
#include <string>
#include <stdint.h>
#include <boost/asio/ip/udp.hpp>
#include <showtime/ZstConstants.h>
#include <showtime/ZstExports.h>

// RFC 5389 Section 6 STUN Message Structure
struct STUNMessageHeader
{
	// Message Type (Binding Request / Response)
	unsigned short type;

	// Payload length of this message
	unsigned short length;

	// Magic Cookie
	unsigned int cookie;

	// Unique Transaction ID
	unsigned int identifier[3];
};


// RFC 5389 Section 15 STUN Attributes
struct STUNAttributeHeader
{
	// Attibute Type
	unsigned short type;

	// Payload length of this attribute
	unsigned short length;
};

#define IPv4_ADDRESS_FAMILY 0x01
#define IPv6_ADDRESS_FAMILY 0x02

#define STUN_MAGIC_COOKIE 0x2112A442
#define STUN_XOR_PORT_COOKIE 0x2112

#define STUN_MSG_BINDING_REQUEST 0x0001
#define STUN_MSG_BINDING_RESPONSE 0x0101
#define STUN_MSG_BINDING_ERR 0x0111

#define STUN_ATTR_MAPPED_ADDRESS 0x0001
#define STUN_ATTR_XOR_MAPPED_ADDRESS 0x0020

// Socket options
typedef boost::asio::detail::socket_option::integer<SOL_SOCKET, SO_RCVTIMEO> rcv_timeout_option; //somewhere in your headers to be used everywhere you need it
typedef boost::asio::detail::socket_option::boolean<SOL_SOCKET, SO_BROADCAST> rcv_broadcast;
#ifdef WIN32
typedef boost::asio::detail::socket_option::boolean<SOL_SOCKET, SO_REUSEADDR> rcv_reuseaddr;
#endif

// RFC 5389 Section 15.2 XOR-MAPPED-ADDRESS
struct STUNXORMappedIPv4Address
{
	unsigned char reserved;
	unsigned char family;
	unsigned short port;
	unsigned int address;
};

enum class STUNError {
	BAD_MESSAGE = -2,
	IDENTIFIER_MISMATCH = -1,
	NO_ADDRESS_FOUND = 0,
	VALID = 1
};

namespace showtime {
	class ZstSTUNService
	{
	public:
		struct STUNServer
		{
			std::string address;
			uint16_t port;
			uint16_t local_port;
		};

		ZST_EXPORT ZstSTUNService();
		ZST_EXPORT ~ZstSTUNService();
		ZST_EXPORT STUNError getPublicIPAddressFromResponse(std::string& out_address, const char* reply, size_t reply_length, unsigned int request_identifier = 0);
		ZST_EXPORT static std::string local_ip();
	private:
		STUNMessageHeader* createSTUNRequest();
	};

}
