#include <ZstEvents.h>

#include <ZstSynchronisable.h>
#include <entities/ZstPerformer.h>
#include <entities/ZstEntityBase.h>
#include <entities/ZstComponent.h>
#include <entities/ZstContainer.h>
#include <entities/ZstPlug.h>
#include <ZstCable.h>

ZstSynchronisableEvent::ZstSynchronisableEvent(ZstSynchronisable * target) :ZstEvent(target)
{
}

ZstEntityEvent::ZstEntityEvent(ZstEntityBase * target) : ZstEvent(target)
{
}

ZstComponentEvent::ZstComponentEvent(ZstComponent * target) : ZstEvent(target)
{
}

ZstContainerEvent::ZstContainerEvent(ZstContainer * target) : ZstEvent(target)
{
}

ZstPlugEvent::ZstPlugEvent(ZstPlug * target) : ZstEvent(target)
{
}

ZstCableEvent::ZstCableEvent(ZstCable * target) : ZstEvent(target)
{
}

ZstPerformerEvent::ZstPerformerEvent(ZstPerformer * target) : ZstEvent(target)
{
}
