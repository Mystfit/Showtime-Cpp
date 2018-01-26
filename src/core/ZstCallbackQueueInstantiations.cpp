#include "ZstCable.h"
#include "ZstURI.h"
#include "entities/ZstPlug.h"
#include "entities/ZstPerformer.h"
#include "entities/ZstEntityBase.h"
#include "ZstEvents.h"
#include "ZstCallbackQueue.h"

template class ZstCallbackQueue<ZstSynchronisableEvent, ZstSynchronisable*>;
template class ZstCallbackQueue<ZstEntityEvent, ZstEntityBase*>;
template class ZstCallbackQueue<ZstComponentEvent, ZstComponent*>;
template class ZstCallbackQueue<ZstComponentTypeEvent, ZstComponent*>;
template class ZstCallbackQueue<ZstCableEvent, ZstCable*>;
template class ZstCallbackQueue<ZstPlugEvent, ZstPlug*>;
template class ZstCallbackQueue<ZstPerformerEvent, ZstPerformer*>;
