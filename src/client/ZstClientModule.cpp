#include "ZstClientModule.h"

void ZstClientModule::set_wake_condition(std::weak_ptr<ZstSemaphore> condition)
{
	m_stage_events.set_wake_condition(condition);
	m_module_events.set_wake_condition(condition);
}

ZstEventDispatcher<ZstTransportAdaptor*>& ZstClientModule::stage_events()
{
	return m_stage_events;
}

ZstEventDispatcher<ZstModuleAdaptor*>& ZstClientModule::module_events()
{
	return m_module_events;
}
