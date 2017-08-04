#include "ZstCallbackQueue.h"
#include "ZstEvent.h"
#include "ZstPerformer.h"
#include "ZstCable.h"
#include "ZstPlug.h"

template class ZstCallbackQueue<ZstEventCallback, ZstEvent>;
template class ZstCallbackQueue<ZstEntityEventCallback, ZstURI>;
template class ZstCallbackQueue<ZstCableEventCallback, ZstCable>;
template class ZstCallbackQueue<ZstPlugEventCallback, ZstURI>;
template class ZstCallbackQueue<ZstPlugDataEventCallback, ZstInputPlug*>;