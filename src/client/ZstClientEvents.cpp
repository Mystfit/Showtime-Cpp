#include "ZstClientEvents.h"
#include "ZstClient.h"

// ------------------
// Item removal hooks
// ------------------

void ZstPlugLeavingEvent::run(ZstPlug * target)
{
	ZstClient::instance().destroy_plug_complete(ZstMsgKind::OK, target);
}

void ZstCableLeavingEvent::run(ZstCable * target)
{
	ZstClient::instance().destroy_cable_complete(ZstMsgKind::OK, target);
}

void ZstSynchronisableDeferredEvent::run(ZstSynchronisable * target)
{
	target->process_events();
}

void ZstComputeEvent::run(ZstInputPlug * target)
{
	try {
		dynamic_cast<ZstComponent*>(target->parent())->compute(target);
	}
	catch (std::exception e) {
		ZstLog::entity(LogLevel::error, "Compute on component {} failed.", target->parent()->URI().path());
	}
}

void ZstEntityLeavingEvent::run(ZstEntityBase * target)
{
	ZstClient::instance().destroy_entity_complete(ZstMsgKind::OK, target);
}
