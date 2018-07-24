#pragma once

#include <ZstEventDispatcher.hpp>
#include "../core/adaptors/ZstTransportAdaptor.hpp"

class ZstClientModule {
public:
	ZstEventDispatcher<ZstTransportAdaptor*> & stage_events();
	ZstEventDispatcher<ZstTransportAdaptor*> & performance_events();

private:
	ZstEventDispatcher<ZstTransportAdaptor*> m_stage_events;
	ZstEventDispatcher<ZstTransportAdaptor*> m_performance_events;
};