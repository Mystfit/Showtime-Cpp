#pragma once

#include "ZstExports.h"
#include <ZstConstants.h>

//Forwards
class ZstSynchronisable;

class ZstSynchronisableLiason {
public:
	ZST_EXPORT void synchronisable_enqueue_activation(ZstSynchronisable * synchronisable);
	ZST_EXPORT void synchronisable_enqueue_deactivation(ZstSynchronisable * synchronisable);
	ZST_EXPORT void synchronisable_set_activated(ZstSynchronisable * synchronisable);
	ZST_EXPORT void synchronisable_set_activating(ZstSynchronisable * synchronisable);
	ZST_EXPORT void synchronisable_set_deactivating(ZstSynchronisable * synchronisable);
	ZST_EXPORT void synchronisable_set_activation_status(ZstSynchronisable * synchronisable, ZstSyncStatus status);
	ZST_EXPORT void synchronisable_set_error(ZstSynchronisable * synchronisable, ZstSyncError e);
	ZST_EXPORT void synchronisable_set_destroyed(ZstSynchronisable * synchronisable);
	ZST_EXPORT void synchronisable_set_proxy(ZstSynchronisable * synchronisable);
	ZST_EXPORT void synchronisable_process_events(ZstSynchronisable * synchronisable);
	ZST_EXPORT void synchronisable_annouce_update(ZstSynchronisable * synchronisable);
};