#pragma once

#include <memory>

#include "ZstClientCommon.hpp"

#include "../core/ZstSemaphore.h"
#include "../core/ZstEventDispatcher.hpp"
#include "../core/adaptors/ZstTransportAdaptor.hpp"

class ZstClientModule {
public:
	ZstClientModule();
	void set_wake_condition(std::weak_ptr<ZstSemaphore> condition);
	std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstTransportAdaptor> > >& stage_events();
	virtual void process_events();
	virtual void flush_events();

private:
	std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstTransportAdaptor> > > m_stage_events;
};
