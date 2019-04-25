#include "ZstStageModule.h"

void ZstStageModule::set_wake_condition(std::weak_ptr<ZstSemaphore> condition)
{
	m_router_events.set_wake_condition(condition);
}

ZstEventDispatcher<ZstTransportAdaptor*>& ZstStageModule::router_events()
{
	return m_router_events;
}
