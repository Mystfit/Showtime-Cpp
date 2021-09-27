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
	void set_wake_condition(std::shared_ptr<std::condition_variable>& condition);
	std::shared_ptr<ZstEventDispatcher<ZstStageTransportAdaptor> >& stage_events();
	virtual void process_events();
	virtual void flush_events();

private:
	std::shared_ptr<ZstEventDispatcher<ZstStageTransportAdaptor> > m_stage_events;
};

}
