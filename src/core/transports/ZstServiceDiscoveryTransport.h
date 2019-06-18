#pragma once

#include "ZstExports.h"
#include "../ZstStageMessage.h"
#include "../ZstActor.h"
#include "ZstTransportLayer.h"

//Forwards
typedef struct _zactor_t zactor_t;
typedef struct _zloop_t zloop_t;
typedef struct _zsock_t zsock_t;

class ZstServiceDiscoveryTransport : public ZstTransportLayer<ZstStageMessage> {
public:
    ZstServiceDiscoveryTransport();
    ~ZstServiceDiscoveryTransport();
    void init(int port);
    void destroy() override;
    
    void send_message_impl(ZstMessage * msg) override;
    void on_receive_msg(ZstMessage * msg) override;
    
    //Handle incoming zbeacon messages
    static int s_handle_beacon(zloop_t * loop, zsock_t * socket, void * arg);

    void start_broadcast(const char * message, size_t size, int interval);
    void stop_broadcast();
    
    void start_listening();
    void stop_listening();
    
private:
    void init() override;
    ZstActor m_beacon_actor;
    zactor_t * m_beacon;
};
