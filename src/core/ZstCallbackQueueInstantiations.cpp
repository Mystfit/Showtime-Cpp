#include "ZstCable.h"
#include "ZstURI.h"
#include "entities/ZstPlug.h"
#include "entities/ZstPerformer.h"
#include "entities/ZstEntityBase.h"
#include "ZstCallbacks.h"
#include "ZstCallbackQueue.h"

template class ZstCallbackQueue<ZstPerformerEvent, ZstPerformer*>;
template class ZstCallbackQueue<ZstComponentEvent, ZstEntityBase*>;
template class ZstCallbackQueue<ZstComponentTypeEvent, ZstEntityBase*>;
template class ZstCallbackQueue<ZstCableEvent, ZstCable*>;
template class ZstCallbackQueue<ZstPlugEvent, ZstPlug*>;
