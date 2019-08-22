#pragma once

#include "ZstExports.h"
#include "adaptors/ZstEventAdaptor.hpp"
#include "ZstCable.h"
#include "ZstServerAddress.h"

//Forwards
class ZstHierarchy;

class ZST_EXPORT ZstSessionAdaptor : public ZstEventAdaptor {
public:
	ZST_EXPORT virtual void on_connected_to_stage();
	ZST_EXPORT virtual void on_disconnected_from_stage();
	ZST_EXPORT virtual void on_synchronised_with_stage();

    ZST_EXPORT virtual void on_server_discovered(const ZstServerAddress & server);

	ZST_EXPORT virtual void on_cable_created(ZstCable * cable);
	ZST_EXPORT virtual void on_cable_destroyed(ZstCable * cable);
    
    ZST_EXPORT virtual ZstCableBundle & get_cables(ZstCableBundle & bundle);
    ZST_EXPORT virtual ZstCable * find_cable(const ZstCableAddress & address);
	
    ZST_EXPORT virtual void aquire_entity_ownership(ZstEntityBase * entity);
	ZST_EXPORT virtual void release_entity_ownership(ZstEntityBase * entity);
    
    ZST_EXPORT virtual ZstHierarchy * hierarchy();
};
