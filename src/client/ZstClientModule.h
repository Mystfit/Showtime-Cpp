#pragma once

#include "../core/ZstEventDispatcher.hpp"
#include "../core/adaptors/ZstTransportAdaptor.hpp"
#include <memory>

//Forwards
class ZstEventWakeup;

class ZstClientModule {
public:
	void set_wake_condition(std::shared_ptr<ZstEventWakeup> condition);
	ZstEventDispatcher<ZstTransportAdaptor*> & stage_events();
	ZstEventDispatcher<ZstTransportAdaptor*> & performance_events();

private:
	ZstEventDispatcher<ZstTransportAdaptor*> m_stage_events;
	ZstEventDispatcher<ZstTransportAdaptor*> m_performance_events;
};
