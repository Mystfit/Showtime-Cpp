#include <showtime/entities/ZstEntityFactory.h>
#include <showtime/entities/ZstEntityBase.h>
#include "ZstEntityFactoryLiason.hpp"


namespace showtime {

ZstEntityBase * ZstEntityFactoryLiason::factory_activate_entity(ZstEntityFactory * factory, ZstEntityBase * entity)
{
	return factory->activate_entity(entity);
}

}
