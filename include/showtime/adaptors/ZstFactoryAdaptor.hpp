#pragma once

#include <showtime/ZstExports.h>
#include <showtime/adaptors/ZstEventAdaptor.hpp>

namespace showtime {

class ZST_CLASS_EXPORTED ZstFactoryAdaptor :
	public ZstEventAdaptor
{
public:
	MULTICAST_DELEGATE_OneParam(CreateablesUpdated, ZstEntityFactory*)
	ZST_EXPORT virtual void on_creatables_updated(ZstEntityFactory * factory);

	MULTICAST_DELEGATE_OneParam(EntityCreated, ZstEntityBase*)
	ZST_EXPORT virtual void on_entity_created(ZstEntityBase * entity);
};

}
