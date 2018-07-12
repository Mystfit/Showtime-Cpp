#include <algorithm>
#include <assert.h>
#include <ZstSynchronisable.h>
#include <ZstEventDispatcher.hpp>

ZstSynchronisable::ZstSynchronisable() :
	m_is_destroyed(false),
	m_sync_status(ZstSyncStatus::DEACTIVATED),
    m_sync_error(ZstSyncError::NO_ERR),
	m_is_proxy(false)
{
	m_synchronisable_events = new ZstEventDispatcher<ZstSynchronisableAdaptor*>("synchronisable");
}

ZstSynchronisable::~ZstSynchronisable()
{
	m_synchronisable_events->flush();
	m_synchronisable_events->remove_all_adaptors();
	delete m_synchronisable_events;
}

void ZstSynchronisable::add_adaptor(ZstSynchronisableAdaptor * adaptor)
{
	m_synchronisable_events->add_adaptor(adaptor);

	//If we are already activated, immediately dispatch an event
	if (is_activated()) {
		m_synchronisable_events->invoke([this](ZstSynchronisableAdaptor* dlg) { dlg->on_synchronisable_activated(this); });
	}
}

void ZstSynchronisable::remove_adaptor(ZstSynchronisableAdaptor * adaptor)
{
	m_synchronisable_events->remove_adaptor(adaptor);
}

ZstSynchronisable::ZstSynchronisable(const ZstSynchronisable & other) : ZstSynchronisable()
{
	m_sync_status = other.m_sync_status;
	m_is_destroyed = other.m_is_destroyed;
	m_is_proxy = other.m_is_proxy;
}

void ZstSynchronisable::enqueue_activation()
{
	if (is_deactivated() || activation_status() != ZstSyncStatus::ACTIVATED)
	{
		set_activation_status(ZstSyncStatus::ACTIVATED);

		//Notify adaptors synchronisable is activating
		m_synchronisable_events->defer([this](ZstSynchronisableAdaptor* dlg) { 
			dlg->on_synchronisable_activated(this);
			this->on_activation();
		});

		//Notify adaptors that we have a queued event
		m_synchronisable_events->invoke([this](ZstSynchronisableAdaptor* dlg) { dlg->synchronisable_has_event(this); });
	}
}

void ZstSynchronisable::enqueue_deactivation()
{
    if (is_activated() || activation_status() != ZstSyncStatus::DEACTIVATED)
    {
        set_activation_status(ZstSyncStatus::DEACTIVATED);

		//Notify adaptors synchronisable is deactivating
		m_synchronisable_events->defer([this](ZstSynchronisableAdaptor* dlg) { 
			dlg->on_synchronisable_deactivated(this); 
			this->on_deactivation();
		});

		//Notify adaptors that we have a queued event
		m_synchronisable_events->invoke([this](ZstSynchronisableAdaptor* dlg) { dlg->synchronisable_has_event(this); });

		//Notify adaptors that this syncronisable needs to be cleaned up
		m_synchronisable_events->invoke([this](ZstSynchronisableAdaptor * dlg) {dlg->on_synchronisable_destroyed(this); });
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
		break;
	case ACTIVATING:
		break;
	case ACTIVATION_QUEUED:
		break;
	case ACTIVATED:
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

void ZstSynchronisable::process_events()
{
	m_synchronisable_events->process_events();
}
