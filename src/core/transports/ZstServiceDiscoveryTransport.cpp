#include "ZstServiceDiscoveryTransport.h"
#include "../ZstZMQRefCounter.h"
#include <czmq.h>

ZstServiceDiscoveryTransport::ZstServiceDiscoveryTransport()
{
    
}

ZstServiceDiscoveryTransport::~ZstServiceDiscoveryTransport()
{
    
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
        char * hostname = zstr_recv(m_beacon);
        ZstLog::net(LogLevel::debug, "Beacon running at {}:{}", hostname, port);
        
        m_beacon_actor.attach_pipe_listener(zactor_sock(m_beacon), s_handle_beacon, this);
    }
    
    m_beacon_actor.start_loop();
}

void ZstServiceDiscoveryTransport::init()
{
    static_assert(true, "Removed: Use init(int port) instead");
}

void ZstServiceDiscoveryTransport::destroy()
{
    ZstTransportLayerBase::destroy();
    
    m_beacon_actor.stop_loop();
    stop_broadcast();
    stop_listening();
    zactor_destroy(&m_beacon);
    zst_zmq_dec_ref_count();
    m_beacon_actor.destroy();
}

void ZstServiceDiscoveryTransport::send_message_impl(ZstMessage * msg)
{
    ZstStageMessage * stage_msg = static_cast<ZstStageMessage*>(msg);
    auto data = stage_msg->as_json_str();
    start_broadcast(data.c_str(), data.size(), 1000);
    release_msg(stage_msg);
}

void ZstServiceDiscoveryTransport::on_receive_msg(ZstMessage * msg)
{
    this->ZstTransportLayerBase::on_receive_msg(msg);
    
    //Publish message to other modules
    msg_events()->defer([msg](ZstTransportAdaptor * adaptor) {
        adaptor->on_receive_msg(msg);
    }, [msg, this](ZstEventStatus status) {
        this->release_msg(static_cast<ZstStageMessage*>(msg));
    });
}

int ZstServiceDiscoveryTransport::s_handle_beacon(zloop_t * loop, zsock_t * socket, void * arg)
{
    ZstServiceDiscoveryTransport * transport = (ZstServiceDiscoveryTransport*)arg;
    char * ipaddress = zstr_recv(socket);
    if (ipaddress) {
        zframe_t * beacon_content = zframe_recv(socket);
        
        auto msg = transport->get_msg();
        auto data = std::string((char*)zframe_data(beacon_content), zframe_size(beacon_content));
        msg->unpack(json::parse(data));
        msg->set_arg(get_msg_arg_name(ZstMsgArg::ADDRESS), ipaddress);
        transport->on_receive_msg(msg);
        zframe_destroy(&beacon_content);
    }
    
    return 0;
}


void ZstServiceDiscoveryTransport::start_broadcast(const char * message, size_t size, int interval)
{
    zsock_send(m_beacon, "sbi", "PUBLISH", message, size, interval);
}

void ZstServiceDiscoveryTransport::stop_broadcast()
{
    zstr_sendx(m_beacon, "SILENCE", NULL);
}

void ZstServiceDiscoveryTransport::start_listening()
{
    zsock_send (m_beacon, "sb", "SUBSCRIBE", "", 0);
}

void ZstServiceDiscoveryTransport::stop_listening()
{
    zstr_sendx(m_beacon, "UNSUBSCRIBE", NULL);
}
