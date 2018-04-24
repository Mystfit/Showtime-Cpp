#include <ZstSynchronisable.h>
#include <algorithm>
#include <assert.h>

ZstSynchronisable::ZstSynchronisable() :
	m_is_destroyed(false),
	m_sync_status(ZstSyncStatus::DEACTIVATED),
    m_sync_error(ZstSyncError::NO_ERR),
	m_is_proxy(false)
{
}

ZstSynchronisable::~ZstSynchronisable()
{
}

void ZstSynchronisable::add_adaptor(ZstSynchronisableAdaptor * adaptor)
{
	ZstEventDispatcher<ZstSynchronisableAdaptor*>::add_adaptor(adaptor);
}

ZstSynchronisable::ZstSynchronisable(const ZstSynchronisable & other) : ZstSynchronisable()
{
	m_sync_status = other.m_sync_status;
	m_is_destroyed = other.m_is_destroyed;
	m_is_proxy = other.m_is_proxy;
}

void ZstSynchronisable::enqueue_activation()
{
	if (is_deactivated() || activation_status() != ZstSyncStatus::ACTIVATION_QUEUED)
	{
		set_activation_status(ZstSyncStatus::ACTIVATION_QUEUED);

		//Notify adaptors synchronisable is activating
		add_event([this](ZstSynchronisableAdaptor* dlg) { dlg->on_synchronisable_activated(this); });

		//Notify adaptors that we have a queued event
		run_event([this](ZstSynchronisableAdaptor* dlg) { dlg->notify_event_ready(this); });
	}
}

void ZstSynchronisable::enqueue_deactivation()
{
    if (is_activated() || activation_status() != ZstSyncStatus::DEACTIVATION_QUEUED)
    {
        set_activation_status(ZstSyncStatus::DEACTIVATION_QUEUED);

		//Notify adaptors synchronisable is deactivating
		add_event([this](ZstSynchronisableAdaptor* dlg) { dlg->on_synchronisable_deactivated(this); });

		//Notify adaptors that we have a queued event
		run_event([this](ZstSynchronisableAdaptor* dlg) { dlg->notify_event_ready(this); });

		//Notify adaptors that this syncronisable needs to be cleaned up
		run_event([this](ZstSynchronisableAdaptor * dlg) {dlg->on_synchronisable_destroyed(this); });
    }
}

void ZstSynchronisable::set_activated()
{
    set_activation_status(ZstSyncStatus::ACTIVATED);
}

void ZstSynchronisable::set_activating()
{
	set_activation_status(ZstSyncStatus::ACTIVATING);
}

void ZstSynchronisable::set_deactivating()
{
	set_activation_status(ZstSyncStatus::DEACTIVATING);
}

void ZstSynchronisable::set_error(ZstSyncError e)
{
	m_sync_status = ZstSyncStatus::ERR;
	m_sync_error = e;
}

bool ZstSynchronisable::is_activated()
{
	return m_sync_status == ZstSyncStatus::ACTIVATED;
}

bool ZstSynchronisable::is_deactivated()
{
	return m_sync_status == ZstSyncStatus::DEACTIVATED;
}

ZstSyncStatus ZstSynchronisable::activation_status()
{
	return m_sync_status;
}

ZstSyncError ZstSynchronisable::last_error()
{
	return m_sync_error;
}

bool ZstSynchronisable::is_destroyed()
{
	return m_is_destroyed;
}

bool ZstSynchronisable::is_proxy()
{
	return m_is_proxy;
}

void ZstSynchronisable::set_activation_status(ZstSyncStatus status)
{
	m_sync_status = status;
	switch (m_sync_status)
	{
	case DEACTIVATED:
		add_event([this](ZstSynchronisableAdaptor* dlg) { dlg->on_synchronisable_deactivated(this); });
		break;
	case ACTIVATING:
		break;
	case ACTIVATION_QUEUED:
		break;
	case ACTIVATED:
		add_event([this](ZstSynchronisableAdaptor* dlg) { dlg->on_synchronisable_activated(this); });
		break;
	case DEACTIVATING:
		break;
	case DEACTIVATION_QUEUED:
		break;
	case ERR:
		break;
	default:
		break;
	}
}

void ZstSynchronisable::set_destroyed()
{
	m_is_destroyed = true;
}

void ZstSynchronisable::set_proxy()
{
	m_is_proxy = true;
}
