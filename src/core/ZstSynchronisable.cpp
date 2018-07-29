#include <algorithm>
#include <assert.h>
#include <ZstSynchronisable.h>
#include <ZstEventDispatcher.hpp>

unsigned int ZstSynchronisable::s_instance_id_counter = 0;

ZstSynchronisable::ZstSynchronisable() :
	m_is_destroyed(false),
	m_sync_status(ZstSyncStatus::DEACTIVATED),
    m_sync_error(ZstSyncError::NO_ERR),
	m_is_proxy(false)
{
	m_instance_id = ++ZstSynchronisable::s_instance_id_counter;
	m_synchronisable_events = new ZstEventDispatcher<ZstSynchronisableAdaptor*>("synchronisable");
}

ZstSynchronisable::ZstSynchronisable(const ZstSynchronisable & other) : ZstSynchronisable()
{
	m_sync_status = other.m_sync_status;
	m_is_destroyed = other.m_is_destroyed;
	m_is_proxy = other.m_is_proxy;
	m_instance_id = ++ZstSynchronisable::s_instance_id_counter;
}

ZstSynchronisable::~ZstSynchronisable()
{
	m_synchronisable_events->flush();
	m_synchronisable_events->remove_all_adaptors();
	delete m_synchronisable_events;
}

void ZstSynchronisable::add_adaptor(ZstSynchronisable * self, ZstSynchronisableAdaptor * adaptor)
{
	self->synchronisable_events()->add_adaptor(adaptor);

	//If we are already activated, immediately dispatch an event
	if (self->is_activated()) {
		self->synchronisable_events()->invoke([self](ZstSynchronisableAdaptor* dlg) { dlg->on_synchronisable_activated(self); });
	}
}

void ZstSynchronisable::remove_adaptor(ZstSynchronisable * self, ZstSynchronisableAdaptor * adaptor)
{
	self->synchronisable_events()->remove_adaptor(adaptor);
}

void ZstSynchronisable::enqueue_activation()
{
	if (is_deactivated() || activation_status() != ZstSyncStatus::ACTIVATED)
	{
		set_activation_status(ZstSyncStatus::ACTIVATED);

		//Notify adaptors that this synchronisable is activating
		synchronisable_events()->defer([this](ZstSynchronisableAdaptor* dlg) {
			dlg->on_synchronisable_activated(this);
			this->on_activation();
		});

		//Notify adaptors that we have a queued event
		synchronisable_events()->invoke([this](ZstSynchronisableAdaptor* dlg) { dlg->synchronisable_has_event(this); });
	}
}

void ZstSynchronisable::enqueue_deactivation()
{
    if (is_activated() || activation_status() != ZstSyncStatus::DEACTIVATED)
    {
        set_activation_status(ZstSyncStatus::DEACTIVATED);

		//Notify adaptors synchronisable is deactivating
		synchronisable_events()->defer([this](ZstSynchronisableAdaptor* dlg) {
			dlg->on_synchronisable_deactivated(this); 
			this->on_deactivation();
		});

		//Notify adaptors that we have a queued event
		synchronisable_events()->invoke([this](ZstSynchronisableAdaptor* dlg) { dlg->synchronisable_has_event(this); });

		//Notify adaptors that this syncronisable needs to be cleaned up -- proxies only
		if (this->is_proxy())
			synchronisable_events()->invoke([this](ZstSynchronisableAdaptor * dlg) {dlg->on_synchronisable_destroyed(this); });
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

unsigned int ZstSynchronisable::instance_id()
{
	return m_instance_id;
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
	synchronisable_events()->process_events();
}

ZstEventDispatcher<ZstSynchronisableAdaptor*> * ZstSynchronisable::synchronisable_events()
{
    return m_synchronisable_events;
}
