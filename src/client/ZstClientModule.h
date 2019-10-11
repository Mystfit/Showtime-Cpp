#pragma once

#include <memory>

#include "ZstClientCommon.hpp"

#include "../core/ZstSemaphore.h"
#include "../core/ZstEventDispatcher.hpp"
#include "../core/adaptors/ZstStageTransportAdaptor.hpp"

namespace showtime {

class ZstClientModule {
public:
	ZstClientModule();
	void set_wake_condition(std::weak_ptr<ZstSemaphore> condition);
	std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstStageTransportAdaptor> > >& stage_events();
	virtual void process_events();
	virtual void flush_events();

private:
	std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstStageTransportAdaptor> > > m_stage_events;
};

}
