#pragma once

#include "../core/ZstEventDispatcher.hpp"
#include "../core/ZstSemaphore.h"
#include "../core/adaptors/ZstTransportAdaptor.hpp"
#include <memory>


class ZstStageModule {
public:
	virtual void process_events();
	virtual void set_wake_condition(std::weak_ptr<ZstSemaphore> condition);
	ZstEventDispatcher<ZstTransportAdaptor*> & router_events();

private:
	ZstEventDispatcher<ZstTransportAdaptor*> m_router_events;
};
