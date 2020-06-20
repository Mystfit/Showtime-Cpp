#pragma once

#include <showtime/ZstExports.h>
#include <showtime/adaptors/ZstEventAdaptor.hpp>
#include <showtime/ZstServerAddress.h>

namespace showtime {

//Forwards
class ShowtimeClient;

class ZST_CLASS_EXPORTED ZstConnectionAdaptor : 
	public ZstEventAdaptor
{
public:
    ZST_CLIENT_EXPORT virtual void on_connected_to_stage(ShowtimeClient* client, const ZstServerAddress & server);
	ZST_CLIENT_EXPORT virtual void on_disconnected_from_stage(ShowtimeClient* client, const ZstServerAddress & server);
	ZST_CLIENT_EXPORT virtual void on_server_discovered(ShowtimeClient* client, const ZstServerAddress & server);
	ZST_CLIENT_EXPORT virtual void on_server_lost(ShowtimeClient* client, const ZstServerAddress& server);
	ZST_CLIENT_EXPORT virtual void on_synchronised_with_stage(ShowtimeClient* client, const ZstServerAddress & server);
};

}
