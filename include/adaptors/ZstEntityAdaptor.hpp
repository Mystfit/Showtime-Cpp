#pragma once

#include <ZstExports.h>
#include <adaptors/ZstEventAdaptor.hpp>

class ZstEntityBase;

class ZstEntityAdaptor : public ZstEventAdaptor
{
public:
    ZST_EXPORT virtual void publish_entity_update(ZstEntityBase * entity);
};
