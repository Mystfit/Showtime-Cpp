#include "ZstSTUNService.h"
#include <czmq.h>
#include <memory>
#include <boost/exception/diagnostic_information.hpp>
#include <showtime/ZstLogging.h>

using namespace boost::asio::ip;

namespace showtime {
    std::string ZstSTUNService::local_ip()
    {
        ziflist_t* interfaces = ziflist_new();
        std::string interface_ip_str = "127.0.0.1";

        if (ziflist_first(interfaces) != NULL) {
            interface_ip_str = std::string(ziflist_address(interfaces));
        }
        ziflist_destroy(&interfaces);
        return interface_ip_str;
    }

    ZstSTUNService::ZstSTUNService()
    {
        m_io.run();
        m_udp_sock = std::make_shared<boost::asio::ip::udp::socket>(m_io);
    }

    ZstSTUNService::~ZstSTUNService()
    {
        m_io.stop();
    }

    // Modified from https://github.com/0xFireWolf/STUNExternalIP
    std::string ZstSTUNService::getPublicIPAddress(STUNServer server)
    {
        std::string address;

        // Bind socket that we'll be communicating with
        m_udp_sock->open(udp::v4());
        m_udp_sock->non_blocking(false);
        udp::endpoint local_endpoint = boost::asio::ip::udp::endpoint(udp::v4(), server.port);

#ifdef WIN32
        m_udp_sock->set_option(rcv_timeout_option{ 50 });
        m_udp_sock->set_option(rcv_reuseaddr(true));
        m_udp_sock->set_option(rcv_broadcast(true));
#endif
        boost::system::error_code ec;
        m_udp_sock->bind(local_endpoint, ec);
        Log::net(Log::Level::warn, "STUN bind result: {}", ec.message());

        // Remote Address
        // First resolve the STUN server address
        boost::asio::ip::udp::resolver resolver(m_io);
        boost::asio::ip::udp::resolver::query query(server.address, std::to_string(server.port));
        boost::asio::ip::udp::resolver::iterator iter = resolver.resolve(query);
        udp::endpoint remote_endpoint = iter->endpoint();

        // Construct a STUN request
        struct STUNMessageHeader* request = (STUNMessageHeader*)malloc(sizeof(struct STUNMessageHeader));
        request->type = htons(0x0001);
        request->length = htons(0x0000);
        request->cookie = htonl(0x2112A442);

        for (int index = 0; index < 3; index++)
        {
            srand((unsigned int)time(0));
            request->identifier[index] = rand();
        }

        // Send the request
        try {
           m_udp_sock->send_to(boost::asio::buffer(request, sizeof(struct STUNMessageHeader)), remote_endpoint);
        }
        catch(boost::exception const& ex){
            Log::net(Log::Level::debug, "Failed to send data to STUN server {}", boost::diagnostic_information(ex));
            m_udp_sock->close();
            free(request);
            return "";
        }

        // Get reply from TURN server
        char reply[1024];
        udp::endpoint sender_endpoint;
        size_t reply_length = 0;
        size_t iters = 10;
        while (reply_length <= 0 && iters > 0) {
            try {
                reply_length = m_udp_sock->receive_from(boost::asio::buffer(reply, 1024), sender_endpoint);
                if (reply_length > 0) {
                    break;
                }
            }
            catch (boost::exception const& ex) {
                iters--;
            }
        }

        if (reply_length <= 0) {
            Log::net(Log::Level::debug, "No data returned from STUN server");
            m_udp_sock->close();
            free(request);
            return "";
        }

        // Parse STUN server reply
        char* pointer = reply;
        struct STUNMessageHeader* response = (struct STUNMessageHeader*)reply;
        if (response->type == htons(0x0101))
        {
            // Check the identifer
            for (int index = 0; index < 3; index++)
            {
                if (request->identifier[index] != response->identifier[index])
                {
                    m_udp_sock->close();
                    free(request);
                    return "";
                }
            }

            pointer += sizeof(struct STUNMessageHeader);
            while (pointer < reply + reply_length)
            {
                struct STUNAttributeHeader* header = (struct STUNAttributeHeader*)pointer;
                if (header->type == htons(XOR_MAPPED_ADDRESS_TYPE))
                {
                    pointer += sizeof(struct STUNAttributeHeader);
                    struct STUNXORMappedIPv4Address* xorAddress = (struct STUNXORMappedIPv4Address*)pointer;
                    unsigned int numAddress = htonl(xorAddress->address) ^ 0x2112A442;
                    address = fmt::format("{}.{}.{}.{}:{}",
                        (numAddress >> 24) & 0xFF,
                        (numAddress >> 16) & 0xFF,
                        (numAddress >> 8) & 0xFF,
                        numAddress & 0xFF,
                        xorAddress->port);

                    m_udp_sock->close();
                    free(request);
                    return address;
                }

                pointer += (sizeof(struct STUNAttributeHeader) + ntohs(header->length));
            }
        }

        m_udp_sock->close();
        free(request);
        return address;
    }
}
