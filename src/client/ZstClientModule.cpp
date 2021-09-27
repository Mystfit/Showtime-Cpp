#include "ZstClientModule.h"

namespace showtime {

ZstClientModule::ZstClientModule() : 
	m_stage_events(std::make_shared<ZstEventDispatcher<ZstStageTransportAdaptor> >())
{
}

void ZstClientModule::set_wake_condition(std::shared_ptr<std::condition_variable>& condition)
{
	m_stage_events->set_wake_condition(condition);
}

std::shared_ptr<ZstEventDispatcher<ZstStageTransportAdaptor> >& ZstClientModule::stage_events()
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
