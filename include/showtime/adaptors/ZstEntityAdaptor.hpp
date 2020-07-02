#pragma once

#include <showtime/ZstExports.h>
#include <showtime/adaptors/ZstEventAdaptor.hpp>

namespace showtime {
    
class ZstEntityBase;

class ZST_CLASS_EXPORTED ZstEntityAdaptor : 
	public ZstEventAdaptor
{
public:
	MULTICAST_DELEGATE_OneParam(EntityRegistered, ZstEntityBase*)
	ZST_EXPORT virtual void on_entity_registered(ZstEntityBase * entity);

	MULTICAST_DELEGATE_OneParam(RegisterEntity, ZstEntityBase*)
	ZST_EXPORT virtual void on_register_entity(ZstEntityBase* entity);

	MULTICAST_DELEGATE_OneParam(DisconnectCable, const ZstCableAddress&)
	ZST_EXPORT virtual void on_disconnect_cable(const ZstCableAddress& cable);

	// ------

	ZST_EXPORT virtual void publish_entity_update(ZstEntityBase * entity, const ZstURI & original_path);
	ZST_EXPORT virtual void request_entity_registration(ZstEntityBase* entity);
	ZST_EXPORT virtual void request_entity_activation(ZstEntityBase * entity);
};

}
