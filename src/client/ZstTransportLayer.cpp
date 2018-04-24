#include "ZstTransportLayer.h"
#include "ZstMessageDispatcher.h"


ZstTransportLayer::ZstTransportLayer()
{
}

ZstTransportLayer::ZstTransportLayer(ZstMessageDispatcher * dispatcher) :
	m_msg_dispatch(dispatcher)
{
}

ZstTransportLayer::~ZstTransportLayer()
{
}
