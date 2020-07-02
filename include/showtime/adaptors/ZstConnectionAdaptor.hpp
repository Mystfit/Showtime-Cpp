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
	MULTICAST_DELEGATE_TwoParams(ConnectedToStage, ShowtimeClient*, const ZstServerAddress&)
    ZST_CLIENT_EXPORT virtual void on_connected_to_stage(ShowtimeClient* client, const ZstServerAddress & server);

	MULTICAST_DELEGATE_TwoParams(DisconnectedFromStage, ShowtimeClient*, const ZstServerAddress&)
	ZST_CLIENT_EXPORT virtual void on_disconnected_from_stage(ShowtimeClient* client, const ZstServerAddress & server);

	MULTICAST_DELEGATE_TwoParams(ServerDisconnected, ShowtimeClient*, const ZstServerAddress&)
	ZST_CLIENT_EXPORT virtual void on_server_discovered(ShowtimeClient* client, const ZstServerAddress & server);

	MULTICAST_DELEGATE_TwoParams(ServerLost, ShowtimeClient*, const ZstServerAddress&)
	ZST_CLIENT_EXPORT virtual void on_server_lost(ShowtimeClient* client, const ZstServerAddress& server);

	MULTICAST_DELEGATE_TwoParams(SynchronisedWithStage, ShowtimeClient*, const ZstServerAddress&)
	ZST_CLIENT_EXPORT virtual void on_synchronised_with_stage(ShowtimeClient* client, const ZstServerAddress & server);
};

}
