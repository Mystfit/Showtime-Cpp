#pragma once

#include "ZstExports.h"
#include "entities/ZstEntityBase.h"

#include "adaptors/ZstSessionAdaptor.hpp"
#include "../ZstEventDispatcher.hpp"

namespace showtime {

class ZstEntityLiason {
public:
    ZST_EXPORT void entity_set_owner(ZstEntityBase * entity, const ZstURI & owner);
};

}
