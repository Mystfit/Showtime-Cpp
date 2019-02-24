#include "ZstEntityFactoryLiason.hpp"
#include "entities/ZstEntityFactory.h"
#include "entities/ZstEntityBase.h"

ZstEntityBase * ZstEntityFactoryLiason::factory_activate_entity(ZstEntityFactory * factory, ZstEntityBase * entity)
{
	return factory->activate_entity(entity);
}
