#pragma once

#include <showtime/ZstExports.h>
#include <showtime/adaptors/ZstEventAdaptor.hpp>
#include <showtime/ZstCable.h>
#include <showtime/ZstServerAddress.h>
#include <memory>

namespace showtime {

//Forwards
class ZstEntityBase;
class ZstCable;

class ZST_CLASS_EXPORTED ZstSessionAdaptor
#ifndef SWIG
	: public inheritable_enable_shared_from_this< ZstSessionAdaptor >
#endif
{
public:
	ZST_EXPORT ZstSessionAdaptor();
	MULTICAST_DELEGATE_OneParam(ZST_EXPORT, cable_created, ZstCable*, cable)
	MULTICAST_DELEGATE_OneParam(ZST_EXPORT, cable_destroyed, const ZstCableAddress&, cable_address)

	//----
	ZST_EXPORT virtual ZstCable* connect_cable(ZstInputPlug* input, ZstOutputPlug* output, const ZstTransportRequestBehaviour& sendtype) { return nullptr; };

	ZST_EXPORT virtual ZstCableBundle& get_cables(ZstCableBundle& bundle) { return bundle; };
	ZST_EXPORT virtual ZstCable* find_cable(const ZstCableAddress& address) { return nullptr; };
	ZST_EXPORT virtual void destroy_cable(ZstCable* cable) {};

	ZST_EXPORT virtual void aquire_entity_ownership(ZstEntityBase* entity) {};
	ZST_EXPORT virtual void release_entity_ownership(ZstEntityBase* entity) {};
	ZST_EXPORT virtual void plug_received_value(ZstInputPlug* plug) {};
};

}
