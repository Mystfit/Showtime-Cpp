#include "ZstClientModule.h"

ZstClientModule::ZstClientModule() : 
	m_stage_events(std::make_shared<ZstEventDispatcher<std::shared_ptr<ZstTransportAdaptor> > >())
{
}

void ZstClientModule::set_wake_condition(std::weak_ptr<ZstSemaphore> condition)
{
	m_stage_events->set_wake_condition(condition);
}

std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstTransportAdaptor> > >& ZstClientModule::stage_events()
{
	return m_stage_events;
}
	
void ZstClientModule::process_events()
{
	m_stage_events->process_events();
}

void ZstClientModule::flush_events()
{
	m_stage_events->flush();
}
