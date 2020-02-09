#pragma once

#include "ZstExports.h"
#include "ZstTransportLayerBase.hpp"
#include "ZstStageTransport.h"
#include "../adaptors/ZstServiceDiscoveryAdaptor.hpp"
#include "../ZstServerBeaconMessage.h"
#include "../ZstActor.h"

namespace showtime {

//Forwards
typedef struct _zactor_t zactor_t;
typedef struct _zloop_t zloop_t;
typedef struct _zsock_t zsock_t;

class ZstServiceDiscoveryTransport : 
	//public ZstStageTransport
    public ZstTransportLayerBase<ZstServerBeaconMessage, ZstServiceDiscoveryAdaptor>
{
public:
    ZST_EXPORT ZstServiceDiscoveryTransport();
    ZST_EXPORT ~ZstServiceDiscoveryTransport();
    ZST_EXPORT void init(int port);
    ZST_EXPORT virtual void destroy() override;

    //Handle incoming zbeacon messages
    static int s_handle_beacon(zloop_t * loop, zsock_t * socket, void * arg);

    ZST_EXPORT void start_broadcast(const std::string & name, int port, int interval) const;
    ZST_EXPORT void stop_broadcast() const;
    
    ZST_EXPORT void start_listening() const;
    ZST_EXPORT void stop_listening() const;
    
private:
	virtual void send_message_impl(const uint8_t* msg_buffer, size_t msg_buffer_size, const ZstTransportArgs& args) const override {};

    void init() override;
    ZstActor m_beacon_actor;
    zactor_t * m_beacon;
};

}
