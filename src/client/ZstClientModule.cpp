#include "ZstClientModule.h"
#include "../core/ZstEventWakeup.hpp"

void ZstClientModule::set_wake_condition(std::shared_ptr<ZstEventWakeup> condition)
{
	m_stage_events.set_wake_condition(condition);
	m_performance_events.set_wake_condition(condition);
}

ZstEventDispatcher<ZstTransportAdaptor*>& ZstClientModule::stage_events()
{
	return m_stage_events;
}

ZstEventDispatcher<ZstTransportAdaptor*>& ZstClientModule::performance_events()
{
	return m_performance_events;
}
