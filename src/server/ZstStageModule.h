#pragma once

#include "../core/ZstEventDispatcher.hpp"
#include "../core/ZstSemaphore.h"
#include "../core/adaptors/ZstTransportAdaptor.hpp"
#include <memory>

namespace showtime {
class ZstStageModule {
public:
	ZstStageModule();
	virtual void process_events();
	virtual void set_wake_condition(std::weak_ptr<ZstSemaphore> condition);
	std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstTransportAdaptor> > >& router_events();

private:
	std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstTransportAdaptor> > > m_router_events;
};
}
