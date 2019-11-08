#include "ZstStageModule.h"

namespace showtime {

ZstStageModule::ZstStageModule() :
	m_router_events(std::make_shared<ZstEventDispatcher<std::shared_ptr<ZstStageTransportAdaptor> > >())
{
}

void ZstStageModule::process_events()
{
	m_router_events->process_events();
}

void ZstStageModule::set_wake_condition(std::weak_ptr<ZstSemaphore> condition)
{
	m_router_events->set_wake_condition(condition);
}

std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstStageTransportAdaptor> > >& ZstStageModule::router_events()
{
	return m_router_events;
}

}