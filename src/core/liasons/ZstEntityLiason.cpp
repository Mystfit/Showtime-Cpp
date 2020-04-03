#include "ZstEntityLiason.hpp"

namespace showtime {

void ZstEntityLiason::entity_set_owner(ZstEntityBase * entity, const ZstURI & owner)
{
    if(!entity)
        return;
    entity->set_owner(owner);
}

void ZstEntityLiason::entity_set_registered(ZstEntityBase* entity, bool registered)
{
	entity->set_registered(true);
}

void ZstEntityLiason::entity_set_parent(ZstEntityBase* entity, ZstEntityBase* parent)
{
    entity->set_parent(parent);
}

}
