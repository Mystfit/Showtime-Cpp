#include "../ZstZMQRefCounter.h"
#include "schemas/stage_beacon_message_generated.h"

#include <czmq.h>
#include "ZstLogging.h"
#include "ZstServiceDiscoveryTransport.h"

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
    ZstTransportLayerBase::init();

    //Create an actor to handle our zloop
    m_beacon_actor.init("beacon_actor");
    
    //Create beacon actor
    m_beacon = zactor_new(zbeacon, NULL);
    
    if (m_beacon) {
        zst_zmq_inc_ref_count();
        
        zsock_send(m_beacon, "si", "CONFIGURE", port);
        char* hostname = zstr_recv(m_beacon);
        ZstLog::net(LogLevel::debug, "Beacon transport broadcasting on port {}. Hostname is {}", port, hostname);
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
    ZstTransportLayerBase::destroy();
}

int ZstServiceDiscoveryTransport::s_handle_beacon(zloop_t * loop, zsock_t * socket, void * arg)
{
    ZstServiceDiscoveryTransport * transport = (ZstServiceDiscoveryTransport*)arg;
    char * ipaddress = zstr_recv(socket);
    if (ipaddress) {
        auto beacon_content = zframe_recv(socket);
        auto msg = transport->get_msg();
        msg->init(GetStageBeaconMessage(zframe_data(beacon_content)), ipaddress);
        
        //msg->set_arg(get_msg_arg_name(ZstMsgArg::ADDRESS), ipaddress);
        transport->dispatch_receive_event(msg, [beacon_content](ZstEventStatus status){
            zframe_t * b = beacon_content;
            zframe_destroy(&b);
        });
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
	auto beacon_msg = CreateStageBeaconMessage(builder, builder.CreateString(name), m_port, false);
	builder.Finish(beacon_msg);
    zsock_send(m_beacon, "sbi", "PUBLISH", builder.GetBufferPointer(), builder.GetSize(), interval);
}

void ZstServiceDiscoveryTransport::stop_broadcast() const
{
    if (m_beacon) {
        zstr_sendx(m_beacon, "SILENCE", NULL);
        auto builder = FlatBufferBuilder();
        auto beacon_msg = CreateStageBeaconMessage(builder, builder.CreateString(m_name), m_port, true);
        builder.Finish(beacon_msg);
        zsock_send(m_beacon, "sbi", "PUBLISH", builder.GetBufferPointer(), builder.GetSize(), 0);
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
