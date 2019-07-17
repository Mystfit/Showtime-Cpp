#include "ZstEntityLiason.hpp"

void ZstEntityLiason::entity_set_owner(ZstEntityBase * entity, const ZstURI & owner)
{
    if(!entity)
        return;
    entity->set_owner(owner);
}
