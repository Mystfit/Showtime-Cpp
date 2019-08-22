#pragma once

#include "ZstExports.h"
#include "adaptors/ZstEventAdaptor.hpp"

class ZST_EXPORT ZstFactoryAdaptor : public ZstEventAdaptor
{
public:
	ZST_EXPORT virtual void on_creatables_updated(ZstEntityFactory * factory);
	ZST_EXPORT virtual void on_entity_created(ZstEntityBase * entity);
};
