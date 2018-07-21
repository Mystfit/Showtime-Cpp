#include "ZstStageModule.h"

ZstEventDispatcher<ZstTransportAdaptor*>& ZstStageModule::router_events()
{
	return m_router_events;
}

ZstEventDispatcher<ZstTransportAdaptor*>& ZstStageModule::publisher_events()
{
	return m_publisher_events;
}
