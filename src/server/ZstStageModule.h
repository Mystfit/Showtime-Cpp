#pragma once

#include <ZstEventDispatcher.hpp>
#include "../core/adaptors/ZstTransportAdaptor.hpp"

class ZstStageModule {
public:
	ZstEventDispatcher<ZstTransportAdaptor*> & router_events();
	ZstEventDispatcher<ZstTransportAdaptor*> & publisher_events();

private:
	ZstEventDispatcher<ZstTransportAdaptor*> m_router_events;
	ZstEventDispatcher<ZstTransportAdaptor*> m_publisher_events;
};