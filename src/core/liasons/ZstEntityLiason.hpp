#pragma once

#include <showtime/ZstExports.h>
#include <showtime/entities/ZstEntityBase.h>
#include <showtime/adaptors/ZstSessionAdaptor.hpp>
#include "../ZstEventDispatcher.hpp"

namespace showtime {

class ZstEntityLiason {
public:
    ZST_EXPORT void entity_set_owner(ZstEntityBase * entity, const ZstURI & owner);
	ZST_EXPORT void entity_set_registered(ZstEntityBase* entity, bool registered);
	ZST_EXPORT void entity_set_parent(ZstEntityBase* entity, ZstEntityBase* parent);
};

}
