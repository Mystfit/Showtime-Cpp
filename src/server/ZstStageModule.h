#pragma once

#include "../core/ZstEventDispatcher.hpp"
#include "../core/adaptors/ZstTransportAdaptor.hpp"
#include <memory>

//Forwards
class ZstEventWakeup;

class ZstStageModule {
public:
	void set_wake_condition(std::shared_ptr<ZstEventWakeup> condition);
	ZstEventDispatcher<ZstTransportAdaptor*> & router_events();
	ZstEventDispatcher<ZstTransportAdaptor*> & publisher_events();

private:
	ZstEventDispatcher<ZstTransportAdaptor*> m_router_events;
	ZstEventDispatcher<ZstTransportAdaptor*> m_publisher_events;
};
