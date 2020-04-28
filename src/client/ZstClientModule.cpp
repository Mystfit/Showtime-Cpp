#include "ZstClientModule.h"

namespace showtime {

ZstClientModule::ZstClientModule() : 
	m_stage_events(std::make_shared<ZstEventDispatcher<std::shared_ptr<ZstStageTransportAdaptor> > >())
{
}

void ZstClientModule::set_wake_condition(std::shared_ptr<ZstSemaphore>& condition)
{
	m_stage_events->set_wake_condition(condition);
}

std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstStageTransportAdaptor> > >& ZstClientModule::stage_events()
{
	return m_stage_events;
}
	
void ZstClientModule::process_events()
{
	m_stage_events->process_events();
}

void ZstClientModule::flush_events()
{
}

}
