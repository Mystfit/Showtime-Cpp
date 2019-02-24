#pragma once

#include <memory>

#include "../core/ZstEventDispatcher.hpp"
#include "../core/adaptors/ZstTransportAdaptor.hpp"
#include "../core/adaptors/ZstModuleAdaptor.hpp"

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
