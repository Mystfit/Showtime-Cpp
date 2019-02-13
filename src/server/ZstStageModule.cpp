#include "ZstStageModule.h"
#include "../core/ZstEventWakeup.hpp"

void ZstStageModule::set_wake_condition(std::shared_ptr<ZstEventWakeup> condition)
{
	m_router_events.set_wake_condition(condition);
}

ZstEventDispatcher<ZstTransportAdaptor*>& ZstStageModule::router_events()
{
	return m_router_events;
}
