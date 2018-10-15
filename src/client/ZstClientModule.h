#pragma once

#include "../core/ZstEventDispatcher.hpp"
#include "../core/adaptors/ZstTransportAdaptor.hpp"
#include "../core/adaptors/ZstModuleAdaptor.hpp"
#include <memory>

//Forwards
class ZstEventWakeup;

class ZstClientModule {
public:
	void set_wake_condition(std::shared_ptr<ZstEventWakeup> condition);
	ZstEventDispatcher<ZstTransportAdaptor*> & stage_events();
	ZstEventDispatcher<ZstModuleAdaptor*> & module_events();

private:
	ZstEventDispatcher<ZstTransportAdaptor*> m_stage_events;
	ZstEventDispatcher<ZstModuleAdaptor*> m_module_events;
};
