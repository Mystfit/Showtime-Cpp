#pragma once

#include <ZstExports.h>
#include <adaptors/ZstEventAdaptor.hpp>

class ZstEntityBase;

class ZstEntityAdaptor : public ZstEventAdaptor
{
public:
	ZST_EXPORT virtual void on_entity_destroyed(ZstEntityBase * entity);
	ZST_EXPORT virtual void on_publish_entity_update(ZstEntityBase * entity);
	ZST_EXPORT virtual void on_register_entity(ZstEntityBase * entity);
};
