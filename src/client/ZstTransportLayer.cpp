# include "ZstTransportLayer.h"
#include "ZstClient.h"


ZstTransportLayer::ZstTransportLayer()
{
}

ZstTransportLayer::ZstTransportLayer(ZstClient * client) :
	ZstClientModule(client)
{
}

ZstTransportLayer::~ZstTransportLayer()
{
}
