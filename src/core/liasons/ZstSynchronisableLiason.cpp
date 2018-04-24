#include "ZstSynchronisableLiason.hpp"
#include <ZstSynchronisable.h>

void ZstSynchronisableLiason::synchronisable_enqueue_activation(ZstSynchronisable * synchronisable)
{
	synchronisable->enqueue_activation();
}

void ZstSynchronisableLiason::synchronisable_enqueue_deactivation(ZstSynchronisable * synchronisable)
{
	synchronisable->enqueue_deactivation();
}

void ZstSynchronisableLiason::synchronisable_set_activated(ZstSynchronisable * synchronisable)
{
	synchronisable->set_activated();
}

void ZstSynchronisableLiason::synchronisable_set_activating(ZstSynchronisable * synchronisable)
{
	synchronisable->set_activating();
}

void ZstSynchronisableLiason::synchronisable_set_deactivating(ZstSynchronisable * synchronisable)
{
	synchronisable->set_deactivating();
}

void ZstSynchronisableLiason::synchronisable_set_activation_status(ZstSynchronisable * synchronisable, ZstSyncStatus status)
{
	synchronisable->set_activation_status(status);
}

void ZstSynchronisableLiason::synchronisable_set_error(ZstSynchronisable * synchronisable, ZstSyncError e)
{
	synchronisable->set_error(e);
}

void ZstSynchronisableLiason::synchronisable_set_destroyed(ZstSynchronisable * synchronisable)
{
	synchronisable->set_destroyed();
}

void ZstSynchronisableLiason::synchronisable_set_proxy(ZstSynchronisable * synchronisable)
{
	synchronisable->set_proxy();
}
