#pragma once

#include "ZstExports.h"
#include "adaptors/ZstEventAdaptor.hpp"

class ZstEntityBase;

class ZST_EXPORT ZstEntityAdaptor : public ZstEventAdaptor
{
public:
	ZST_EXPORT virtual void on_publish_entity_update(ZstEntityBase * entity);
	ZST_EXPORT virtual void on_register_entity(ZstEntityBase * entity);
	ZST_EXPORT virtual void on_request_entity_activation(ZstEntityBase * entity);
    ZST_EXPORT virtual void on_disconnect_cable(ZstCable * cable);
};
