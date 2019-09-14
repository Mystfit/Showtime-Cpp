#pragma once

#include "ZstExports.h"
#include <ZstConstants.h>

namespace showtime {

//Forwards
class ZstEntityFactory;
class ZstEntityBase;

class ZstEntityFactoryLiason {
public:
	ZST_EXPORT ZstEntityBase * factory_activate_entity(ZstEntityFactory * factory, ZstEntityBase * entity);
};

}
