#pragma once

#include <showtime/ZstExports.h>
#include <showtime/adaptors/ZstEventAdaptor.hpp>

namespace showtime {

//Forwards
class ZstSynchronisable;

class ZST_CLASS_EXPORTED ZstSynchronisableAdaptor : 
	public ZstEventAdaptor<ZstSynchronisableAdaptor>
{
public:
	MULTICAST_DELEGATE_OneParam(ZST_EXPORT, synchronisable_activated, ZstSynchronisable*, synchronisable)
	MULTICAST_DELEGATE_OneParam(ZST_EXPORT, synchronisable_deactivated, ZstSynchronisable*, synchronisable)
	MULTICAST_DELEGATE_TwoParams(ZST_EXPORT, synchronisable_destroyed, ZstSynchronisable*, synchronisable, bool, already_removed = false)
	MULTICAST_DELEGATE_OneParam(ZST_EXPORT, synchronisable_updated, ZstSynchronisable*, synchronisable)

	// ------

	ZST_EXPORT virtual void synchronisable_has_event(ZstSynchronisable * synchronisable);
};

}
