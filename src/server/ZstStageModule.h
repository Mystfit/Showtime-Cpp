#pragma once

#include "../core/ZstEventDispatcher.hpp"
#include "../core/adaptors/ZstTransportAdaptor.hpp"
#include <memory>

//Forwards
class ZstEventWakeup;

class ZstStageModule {
public:
	void set_wake_condition(std::weak_ptr<ZstEventWakeup> condition);
	ZstEventDispatcher<ZstTransportAdaptor*> & router_events();

private:
	ZstEventDispatcher<ZstTransportAdaptor*> m_router_events;
};
