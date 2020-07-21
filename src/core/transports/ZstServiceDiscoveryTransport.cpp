#include <czmq.h>
#include <showtime/schemas/messaging/stage_beacon_message_generated.h>
#include <showtime/ZstLogging.h>
#include "ZstServiceDiscoveryTransport.h"
#include "../ZstZMQRefCounter.h"

using namespace flatbuffers;

namespace showtime {

ZstServiceDiscoveryTransport::ZstServiceDiscoveryTransport() : 
    m_beacon(NULL),
    m_name(""),
    m_port(0)
{
    
}

ZstServiceDiscoveryTransport::~ZstServiceDiscoveryTransport()
{
	destroy();
}

void ZstServiceDiscoveryTransport::init(int port)
{
    ZstTransportLayer::init();

    //Create an actor to handle our zloop
    m_beacon_actor.init("beacon_actor");

    // Set multicast addresses
    zsys_set_ipv4_mcast_address(CLIENT_MULTICAST_ADDR);
    //zsys_set_interface("*");
    auto mcast_address = zsys_ipv4_mcast_address();
    auto iface = zsys_interface();
    Log::net(Log::Level::debug, "Beacon multicast address is {}. Interface is {}", mcast_address, iface);
    
    //Create beacon actor
    m_beacon = zactor_new(zbeacon, NULL);
    
    if (m_beacon) {
        zst_zmq_inc_ref_count();
        
        zsock_send(m_beacon, "si", "CONFIGURE", port);
        char* hostname = zstr_recv(m_beacon);
        Log::net(Log::Level::debug, "Beacon transport active on port {}", port, hostname);
        zstr_free(&hostname);
        
        m_beacon_actor.attach_pipe_listener(zactor_sock(m_beacon), s_handle_beacon, this);
        m_beacon_actor.start_loop();
		set_connected(true);
    }
}

void ZstServiceDiscoveryTransport::init()
{
    static_assert(true, "Removed: Use init(int port) instead");
}

void ZstServiceDiscoveryTransport::destroy()
{
	stop_broadcast();
	stop_listening();
	m_beacon_actor.stop_loop();
	if (m_beacon) {
		zactor_destroy(&m_beacon);
		zst_zmq_dec_ref_count();
	}
	set_connected(false);
    ZstTransportLayer::destroy();
}

int ZstServiceDiscoveryTransport::s_handle_beacon(zloop_t * loop, zsock_t * socket, void * arg)
{
    ZstServiceDiscoveryTransport * transport = (ZstServiceDiscoveryTransport*)arg;
    char * ipaddress = zstr_recv(socket);
    if (ipaddress) {
        auto beacon_content = zframe_recv(socket);
        auto msg = transport->get_msg();
        
        auto shared_transport = std::static_pointer_cast<ZstServiceDiscoveryTransport>(transport->shared_from_this());
        msg->init(GetStageBeaconMessage(zframe_data(beacon_content)), ipaddress, shared_transport);

        // Set the beacon as having a promise (even if it doesn't) to make sure that the release
        // happens AFTER the beacon has finished processing
        auto cleanup_func = [beacon_content](ZstEventStatus status) {
            zframe_t* b = beacon_content;
            zframe_destroy(&b);
        };

        // Set the has_promise flag to avoid early cleanup
        msg->set_has_promise();
        
        // Let the transport take ownership of cleaning up the message
        auto base_msg = std::static_pointer_cast<ZstMessage>(msg);
        transport->take_message_ownership(base_msg, cleanup_func);

        // Dispatch the message into the event system
        transport->dispatch_receive_event(msg, cleanup_func);

        zstr_free(&ipaddress);
    }
    
    return 0;
}

void ZstServiceDiscoveryTransport::start_broadcast(const std::string& name, int port, int interval)
{
	if (!m_beacon)
		return;

    m_name = name;
    m_port = port;

	auto builder = FlatBufferBuilder();
	auto beacon_msg = CreateStageBeaconMessage(builder, builder.CreateString(name), m_port);
	builder.Finish(beacon_msg);
    zsock_send(m_beacon, "sbi", "PUBLISH", builder.GetBufferPointer(), builder.GetSize(), interval);
}

void ZstServiceDiscoveryTransport::stop_broadcast() const
{
    if (m_beacon) {
        zstr_sendx(m_beacon, "SILENCE", NULL);
    }
}

void ZstServiceDiscoveryTransport::start_listening() const
{
    if(m_beacon)
        zsock_send (m_beacon, "sb", "SUBSCRIBE", "", 0);
}

void ZstServiceDiscoveryTransport::stop_listening() const
{
    if(m_beacon)
        zstr_sendx(m_beacon, "UNSUBSCRIBE", NULL);
}

}
