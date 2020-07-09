#pragma once

#include <showtime/ZstExports.h>
#include <showtime/adaptors/ZstEventAdaptor.hpp>

namespace showtime {

class ZST_CLASS_EXPORTED ZstFactoryAdaptor :
	public ZstEventAdaptor<ZstFactoryAdaptor>
{
public:
	MULTICAST_DELEGATE_OneParam(ZST_EXPORT, creatables_updated, ZstEntityFactory*, factory)
	MULTICAST_DELEGATE_OneParam(ZST_EXPORT, entity_created, ZstEntityBase*, entity)
};

}
