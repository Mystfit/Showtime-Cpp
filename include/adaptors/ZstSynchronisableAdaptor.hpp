#pragma once

#include <ZstExports.h>
#include <adaptors/ZstEventAdaptor.hpp>

//Forwards
class ZstSynchronisable;

class ZstSynchronisableAdaptor : public ZstEventAdaptor {
public:
	ZST_EXPORT virtual void on_synchronisable_activated(ZstSynchronisable * synchronisable);
	ZST_EXPORT virtual void on_synchronisable_deactivated(ZstSynchronisable * synchronisable);
	ZST_EXPORT virtual void on_synchronisable_destroyed(ZstSynchronisable * synchronisable);
	ZST_EXPORT virtual void on_synchronisable_updated(ZstSynchronisable * synchronisable);
	ZST_EXPORT virtual void on_synchronisable_has_event(ZstSynchronisable * synchronisable);
};
