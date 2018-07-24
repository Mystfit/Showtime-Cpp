#include "ZstClientModule.h"

ZstEventDispatcher<ZstTransportAdaptor*>& ZstClientModule::stage_events()
{
	return m_stage_events;
}

ZstEventDispatcher<ZstTransportAdaptor*>& ZstClientModule::performance_events()
{
	return m_performance_events;
}
