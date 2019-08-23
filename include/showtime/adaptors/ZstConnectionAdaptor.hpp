#pragma once

#include "ZstExports.h"
#include "adaptors/ZstEventAdaptor.hpp"
#include "ZstServerAddress.h"

//Forwards
class ShowtimeClient;

class ZstConnectionAdaptor : public ZstEventAdaptor {
public:
    ZST_EXPORT virtual void on_connected_to_stage(ShowtimeClient* client, const ZstServerAddress & server){};
    ZST_EXPORT virtual void on_disconnected_from_stage(ShowtimeClient* client, const ZstServerAddress & server){};
    ZST_EXPORT virtual void on_server_discovered(ShowtimeClient* client, const ZstServerAddress & server){};
    ZST_EXPORT virtual void on_synchronised_with_stage(ShowtimeClient* client, const ZstServerAddress & server){};
};
