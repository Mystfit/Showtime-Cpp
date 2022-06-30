#pragma once
#include <string>
#include <stdint.h>
#include <showtime/ZstConstants.h>
#include <showtime/ZstExports.h>
#include <boost/asio.hpp>

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

#define XOR_MAPPED_ADDRESS_TYPE 0x0020

// RFC 5389 Section 15 STUN Attributes
struct STUNAttributeHeader
{
	// Attibute Type
	unsigned short type;

	// Payload length of this attribute
	unsigned short length;
};

#define IPv4_ADDRESS_FAMILY 0x01;
#define IPv6_ADDRESS_FAMILY 0x02;

typedef boost::asio::detail::socket_option::integer<SOL_SOCKET, SO_RCVTIMEO> rcv_timeout_option; //somewhere in your headers to be used everywhere you need it

// RFC 5389 Section 15.2 XOR-MAPPED-ADDRESS
struct STUNXORMappedIPv4Address
{
	unsigned char reserved;
	unsigned char family;
	unsigned short port;
	unsigned int address;
};

namespace showtime {
	class ZstSTUNService
	{
	public:
		struct STUNServer
		{
			std::string address;
			uint16_t port;
		};

		ZST_EXPORT ZstSTUNService();
		ZST_EXPORT ~ZstSTUNService();
		ZST_EXPORT std::string getPublicIPAddress(struct STUNServer server);
		ZST_EXPORT static std::string local_ip();
	private:

		std::shared_ptr<boost::asio::ip::udp::socket> m_udp_sock;
		boost::asio::io_context m_io;
	};

}
