#pragma once

#include "ZstExports.h"
#include "adaptors/ZstEventAdaptor.hpp"

namespace showtime {
    
class ZstEntityBase;

class ZST_CLASS_EXPORTED ZstEntityAdaptor : 
	public ZstEventAdaptor
{
public:
	ZST_EXPORT virtual void on_entity_registered(ZstEntityBase * entity);
	ZST_EXPORT virtual void on_register_entity(ZstEntityBase* entity);
	ZST_EXPORT virtual void on_disconnect_cable(ZstCable* cable);

	// ------

	ZST_EXPORT virtual void publish_entity_update(ZstEntityBase * entity, const ZstURI & original_path);
	ZST_EXPORT virtual void request_entity_registration(ZstEntityBase* entity);
	ZST_EXPORT virtual void request_entity_activation(ZstEntityBase * entity);
};

}
