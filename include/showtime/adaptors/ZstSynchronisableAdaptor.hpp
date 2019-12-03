#pragma once

#include "ZstExports.h"
#include "adaptors/ZstEventAdaptor.hpp"

namespace showtime {

//Forwards
class ZstSynchronisable;

class ZST_CLASS_EXPORTED ZstSynchronisableAdaptor : 
	public ZstEventAdaptor
{
public:
	ZST_EXPORT virtual void on_synchronisable_activated(ZstSynchronisable * synchronisable);
	ZST_EXPORT virtual void on_synchronisable_deactivated(ZstSynchronisable * synchronisable);
	ZST_EXPORT virtual void on_synchronisable_destroyed(ZstSynchronisable * synchronisable, bool already_removed = false);
	ZST_EXPORT virtual void on_synchronisable_updated(ZstSynchronisable * synchronisable);
	ZST_EXPORT virtual void on_synchronisable_has_event(ZstSynchronisable * synchronisable);
};

}
