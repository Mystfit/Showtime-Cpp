#include "ZstServiceDiscoveryTransport.h"
#include "../ZstZMQRefCounter.h"
#include "schemas/stage_beacon_message_generated.h"
#include <czmq.h>

using namespace flatbuffers;

namespace showtime {

ZstServiceDiscoveryTransport::ZstServiceDiscoveryTransport() : m_beacon(NULL)
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
        ZstLog::net(LogLevel::notification, "Beacon transport broadcasting on port {}. Hostname is {}", port, hostname);
        zstr_free(&hostname);
        
        m_beacon_actor.attach_pipe_listener(zactor_sock(m_beacon), s_handle_beacon, this);
        m_beacon_actor.start_loop();
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

void ZstServiceDiscoveryTransport::start_broadcast(const std::string& name, int port, int interval) const
{
	if (!m_beacon)
		return;

	auto builder = FlatBufferBuilder();
	auto beacon_msg = CreateStageBeaconMessage(builder, builder.CreateString(name), port);
	builder.Finish(beacon_msg);
    zsock_send(m_beacon, "sbi", "PUBLISH", builder.GetBufferPointer(), builder.GetSize(), interval);
}

void ZstServiceDiscoveryTransport::stop_broadcast() const
{
    if(m_beacon)
        zstr_sendx(m_beacon, "SILENCE", NULL);
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
