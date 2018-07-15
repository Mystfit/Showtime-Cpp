#pragma once

#include <ZstExports.h>
#include <adaptors/ZstEventAdaptor.hpp>

class ZstEntityBase;

class ZstEntityAdaptor : public ZstEventAdaptor
{
public:
    ZST_EXPORT virtual void entity_publish_update(ZstEntityBase * entity);
};
