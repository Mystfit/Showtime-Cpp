#pragma once

#include <showtime/ZstExports.h>
#include <showtime/adaptors/ZstEventAdaptor.hpp>

namespace showtime {
    
class ZstEntityBase;

class ZST_CLASS_EXPORTED ZstEntityAdaptor
#ifndef SWIG
	: public inheritable_enable_shared_from_this< ZstEntityAdaptor >
#endif
{
public:
	MULTICAST_DELEGATE_OneParam(ZST_CLIENT_EXPORT, entity_registered, ZstEntityBase*, entity)
	MULTICAST_DELEGATE_OneParam(ZST_CLIENT_EXPORT, register_entity, ZstEntityBase*, entity)
	MULTICAST_DELEGATE_OneParam(ZST_CLIENT_EXPORT, disconnect_cable, const ZstCableAddress&, cable)
	MULTICAST_DELEGATE_OneParam(ZST_CLIENT_EXPORT, compute, const ZstPlug*, plug)

	// ------

	ZST_EXPORT virtual void publish_entity_update(ZstEntityBase * entity, const ZstURI & original_path);
	ZST_EXPORT virtual void request_entity_registration(ZstEntityBase* entity);
	ZST_EXPORT virtual void request_entity_activation(ZstEntityBase * entity);
};

}
