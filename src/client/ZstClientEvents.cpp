#include "ZstClientEvents.h"
#include "ZstClient.h"

// ------------------
// Item removal hooks
// ------------------

void ZstSynchronisableDeferredEvent::run(ZstSynchronisable * target)
{
	target->process_events();
}

void ZstComputeEvent::run(ZstInputPlug * target)
{
	ZstComponent * parent = dynamic_cast<ZstComponent*>(target->parent());
	if (!parent) {
		throw std::runtime_error("Could not find parent of input plug");
	}
	try {
		parent->compute(target);
	}
	catch (std::exception e) {
		ZstLog::entity(LogLevel::error, "Compute on component {} failed. Error was: {}", parent->URI().path(), e.what());
	}
}
