#include "ZstTransportLayer.h"
#include "ZstMessageDispatcher.h"


ZstMessageDispatcher * ZstTransportLayer::msg_dispatch()
{
	assert(m_msg_dispatch);
	return m_msg_dispatch;
}

ZstTransportLayer::ZstTransportLayer() : m_msg_dispatch(NULL)
{
}

ZstTransportLayer::~ZstTransportLayer()
{
}

void ZstTransportLayer::set_dispatcher(ZstMessageDispatcher * dispatcher)
{
	m_msg_dispatch = dispatcher;
}

