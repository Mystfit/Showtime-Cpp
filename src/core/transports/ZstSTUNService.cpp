#include "ZstSTUNService.h"
#include <czmq.h>
#include <memory>
#include <boost/exception/diagnostic_information.hpp>
#include <showtime/ZstLogging.h>

using namespace boost::asio::ip;

namespace showtime {

	ZstSTUNService::ZstSTUNService()
	{
		//m_io.run();
		//m_udp_sock = std::make_shared<boost::asio::ip::udp::socket>(m_io);
	}

	ZstSTUNService::~ZstSTUNService()
	{
		//m_io.stop();
	}

    std::string ZstSTUNService::local_ip()
    {
        ziflist_t* interfaces = ziflist_new();
        std::string interface_ip_str = "127.0.0.1";

        if (ziflist_first(interfaces) != NULL) {
            interface_ip_str = std::string(ziflist_address(interfaces));
        }
        ziflist_destroy(&interfaces);
        return interface_ip_str;
        //return "127.0.0.1";
    }

	STUNMessageHeader* ZstSTUNService::createSTUNRequest()
	{
		struct STUNMessageHeader* request = (STUNMessageHeader*)malloc(sizeof(struct STUNMessageHeader));
		request->type = htons(STUN_ATTR_MAPPED_ADDRESS);
		request->length = htons(0x0000);
		request->cookie = htonl(STUN_MAGIC_COOKIE);

		for (int index = 0; index < 3; index++)
		{
			srand((unsigned int)time(0));
			request->identifier[index] = rand();
		}

		return request;
	}

    // Modified from https://github.com/0xFireWolf/STUNExternalIP
   STUNError ZstSTUNService::getPublicIPAddressFromResponse(std::string& out_address, const char* reply, size_t reply_length, unsigned int request_identifier)
    {
	   if (!reply_length || !reply) {
		   return STUNError::BAD_MESSAGE;
	   }

		// Parse STUN server reply
		const char* pointer = reply;
		struct STUNMessageHeader* response = (struct STUNMessageHeader*)reply;
		if (response->type == htons(STUN_MSG_BINDING_RESPONSE))
		{
			// Check the identifer
			for (int index = 0; index < 3; index++)
			{
				if (request_identifier) {
					if (request_identifier != response->identifier[index])
					{
						return STUNError::IDENTIFIER_MISMATCH;
					}
				}
			}

			pointer += sizeof(struct STUNMessageHeader);
			while (pointer < reply + reply_length)
			{
				struct STUNAttributeHeader* header = (struct STUNAttributeHeader*)pointer;
				if (header->type == htons(STUN_ATTR_XOR_MAPPED_ADDRESS))
				{
					pointer += sizeof(struct STUNAttributeHeader);
					struct STUNXORMappedIPv4Address* xorAddress = (struct STUNXORMappedIPv4Address*)pointer;
					unsigned int numAddress = htonl(xorAddress->address) ^ STUN_MAGIC_COOKIE;
					std::string address = fmt::format("{}.{}.{}.{}:{}",
						(numAddress >> 24) & 0xFF,
						(numAddress >> 16) & 0xFF,
						(numAddress >> 8) & 0xFF,
						numAddress & 0xFF,
						ntohs(xorAddress->port) ^ STUN_XOR_PORT_COOKIE);

					out_address = address;
					return STUNError::VALID;
				}

				pointer += (sizeof(struct STUNAttributeHeader) + ntohs(header->length));
			}
		}

		return STUNError::NO_ADDRESS_FOUND;
    }
}
