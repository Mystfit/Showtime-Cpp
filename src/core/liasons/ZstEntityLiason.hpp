#pragma once

#include "ZstExports.h"
#include "entities/ZstEntityBase.h"

#include "adaptors/ZstSessionAdaptor.hpp"
#include "../ZstEventDispatcher.hpp"

class ZstEntityLiason {
public:
    ZST_EXPORT ZstEventDispatcher<ZstSessionAdaptor*> * entity_session_events(ZstEntityBase * entity);
};
