#pragma once

#include "../core/ZstEventDispatcher.hpp"
#include "../core/ZstSemaphore.h"
#include "../core/adaptors/ZstStageTransportAdaptor.hpp"
#include <memory>

namespace showtime {
class ZstStageModule {
public:
	ZstStageModule();
	virtual void process_events();
	virtual void set_wake_condition(std::shared_ptr<ZstSemaphore>& condition);
	std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstStageTransportAdaptor> > >& router_events();

private:
	std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstStageTransportAdaptor> > > m_router_events;
};
}
