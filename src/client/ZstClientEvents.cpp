#include "ZstClientEvents.h"
#include "ZstClient.h"

// ------------------
// Item removal hooks
// ------------------


void ZstComponentLeavingEvent::run(ZstComponent * target)
{
	ZstClient::instance().destroy_entity_complete(static_cast<ZstEntityBase*>(target));
}

void ZstCableLeavingEvent::run(ZstCable * target)
{
	ZstClient::instance().remove_cable(target);
}

void ZstPlugLeavingEvent::run(ZstPlug * target)
{
	dynamic_cast<ZstComponent*>(target->parent())->remove_plug(target);
	ZstClient::instance().destroy_plug(target);
}

void ZstSynchronisableDeferredEvent::run(ZstSynchronisable * target)
{
	target->process_events();
}
