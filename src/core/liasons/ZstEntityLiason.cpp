#include "ZstEntityLiason.hpp"

ZstEventDispatcher<ZstSessionAdaptor*> * ZstEntityLiason::entity_session_events(ZstEntityBase * entity)
{
    return entity->session_events();
}
