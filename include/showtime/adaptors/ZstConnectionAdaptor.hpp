#pragma once

#include <showtime/ZstExports.h>
#include <showtime/adaptors/ZstEventAdaptor.hpp>
#include <showtime/ZstServerAddress.h>

namespace showtime {

//Forwards
class ShowtimeClient;

class ZST_CLASS_EXPORTED ZstConnectionAdaptor
#ifndef SWIG
	: public inheritable_enable_shared_from_this< ZstConnectionAdaptor >
#endif
{
public:
	ZST_EXPORT ZstConnectionAdaptor();
	MULTICAST_DELEGATE_TwoParams(ZST_EXPORT, connected_to_server, ShowtimeClient*, client, const ZstServerAddress*, server)
	MULTICAST_DELEGATE_TwoParams(ZST_EXPORT, disconnected_from_server, ShowtimeClient*, client, const ZstServerAddress*, server)
	MULTICAST_DELEGATE_TwoParams(ZST_EXPORT, server_discovered, ShowtimeClient*, client, const ZstServerAddress*, server)
	MULTICAST_DELEGATE_TwoParams(ZST_EXPORT, server_lost, ShowtimeClient*, client, const ZstServerAddress*, server)
	MULTICAST_DELEGATE_TwoParams(ZST_EXPORT, synchronised_graph, ShowtimeClient*, client, const ZstServerAddress*, server)
};

}
