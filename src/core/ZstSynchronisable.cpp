#include <algorithm>
#include <assert.h>
#include <showtime/ZstSynchronisable.h>
#include "ZstEventDispatcher.hpp"

namespace showtime {

// Template Z
//template class ZstEventDispatcher<std::weak_ptr<ZstSynchronisableAdaptor> >;

// Static vars
unsigned int ZstSynchronisable::s_instance_id_counter = 0;

ZstSynchronisable::ZstSynchronisable() :
	m_is_destroyed(false),
	m_sync_status(ZstSyncStatus::DEACTIVATED),
    m_sync_error(ZstSyncError::NO_ERR),
	m_is_proxy(false),
    m_synchronisable_events(std::make_shared< ZstEventDispatcher<ZstSynchronisableAdaptor> >())
{
	m_instance_id = ++ZstSynchronisable::s_instance_id_counter;
}

ZstSynchronisable::ZstSynchronisable(const ZstSynchronisable & other) :
    m_is_destroyed(other.m_is_destroyed),
	m_sync_status(other.m_sync_status),
	m_is_proxy(other.m_is_proxy),
	m_synchronisable_events(std::make_shared< ZstEventDispatcher<ZstSynchronisableAdaptor> >()),
    m_instance_id(++ZstSynchronisable::s_instance_id_counter)
{
}

ZstSynchronisable::~ZstSynchronisable()
{
}

void ZstSynchronisable::add_adaptor(std::shared_ptr<ZstSynchronisableAdaptor> adaptor)
{
	m_synchronisable_events->add_adaptor(adaptor);

	//If we are already activated, immediately dispatch an event
	if (this->is_activated()) {
		m_synchronisable_events->invoke([this](ZstSynchronisableAdaptor* adaptor) { 
			adaptor->on_synchronisable_activated(this);
		});
	}
}

void ZstSynchronisable::remove_adaptor(std::shared_ptr<ZstSynchronisableAdaptor> adaptor)
{
	m_synchronisable_events->remove_adaptor(adaptor);
}

void ZstSynchronisable::enqueue_activation()
{
	if (this->m_sync_status == ZstSyncStatus::ACTIVATING)
	{
		set_activation_status(ZstSyncStatus::ACTIVATION_QUEUED);

		//Notify adaptors that this synchronisable is activating
		m_synchronisable_events->defer([this](ZstSynchronisableAdaptor* adaptor) {
			this->set_activation_status(ZstSyncStatus::ACTIVATED);
			adaptor->on_synchronisable_activated(this);
		}, [this](ZstEventStatus status) {
			this->on_activation();
		});

		//Notify adaptors that we have a queued event
		m_synchronisable_events->invoke([this](ZstSynchronisableAdaptor* adaptor) {
			adaptor->synchronisable_has_event(this);
		});
	}
}

void ZstSynchronisable::enqueue_deactivation()
{
	if (this->m_sync_status == ZstSyncStatus::DESTROYED)
		return;

    if (this->m_sync_status == ZstSyncStatus::ACTIVATED || this->m_sync_status == ZstSyncStatus::DEACTIVATING)
    {
        set_activation_status(ZstSyncStatus::DEACTIVATION_QUEUED);

		//Notify adaptors synchronisable is deactivating
		m_synchronisable_events->defer([this](ZstSynchronisableAdaptor* adaptor) {
			this->set_activation_status(ZstSyncStatus::DEACTIVATED);
			adaptor->on_synchronisable_deactivated(this);
		}, [this](ZstEventStatus status) {
			this->on_deactivation();
		});

		//Notify adaptors that this syncronisable needs to be cleaned up -- proxies only
		m_synchronisable_events->defer([this](ZstSynchronisableAdaptor* adaptor) {
			adaptor->on_synchronisable_destroyed(this);
        });

		//Notify adaptors that we have a queued event
		m_synchronisable_events->invoke([this](ZstSynchronisableAdaptor* adaptor) {
			adaptor->synchronisable_has_event(this);
        });
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
    std::lock_guard<std::mutex> lock(m_sync_lock);
	m_sync_status = ZstSyncStatus::ERR;
	m_sync_error = e;
}

bool ZstSynchronisable::is_activated()
{
    std::lock_guard<std::mutex> lock(m_sync_lock);
	return m_sync_status == ZstSyncStatus::ACTIVATED;
}

bool ZstSynchronisable::is_deactivated()
{
    std::lock_guard<std::mutex> lock(m_sync_lock);
	return m_sync_status == ZstSyncStatus::DEACTIVATED;
}

ZstSyncStatus ZstSynchronisable::activation_status()
{
    std::lock_guard<std::mutex> lock(m_sync_lock);
	return m_sync_status;
}

ZstSyncError ZstSynchronisable::last_error()
{
    std::lock_guard<std::mutex> lock(m_sync_lock);
	return m_sync_error;
}

bool ZstSynchronisable::is_destroyed()
{
    std::lock_guard<std::mutex> lock(m_sync_lock);
	return m_is_destroyed;
}

bool ZstSynchronisable::is_proxy()
{
    std::lock_guard<std::mutex> lock(m_sync_lock);
	return m_is_proxy;
}

unsigned int ZstSynchronisable::instance_id()
{
    std::lock_guard<std::mutex> lock(m_sync_lock);
	return m_instance_id;
}

void ZstSynchronisable::set_activation_status(ZstSyncStatus status)
{
    std::lock_guard<std::mutex> lock(m_sync_lock);
	m_sync_status = status;
	switch (m_sync_status)
	{
	case ZstSyncStatus::DEACTIVATED:
		break;
	case ZstSyncStatus::ACTIVATING:
		break;
	case ZstSyncStatus::ACTIVATION_QUEUED:
		break;
	case ZstSyncStatus::ACTIVATED:
		break;
	case ZstSyncStatus::DEACTIVATING:
		break;
	case ZstSyncStatus::DEACTIVATION_QUEUED:
		break;
	case ZstSyncStatus::ERR:
		break;
	default:
		break;
	}
}

void ZstSynchronisable::set_deactivated()
{
	set_activation_status(ZstSyncStatus::DEACTIVATED);
}

void ZstSynchronisable::set_destroyed()
{
	set_activation_status(ZstSyncStatus::DESTROYED);
}

void ZstSynchronisable::set_proxy()
{
    std::lock_guard<std::mutex> lock(m_sync_lock);
	m_is_proxy = true;
}

std::shared_ptr<ZstEventDispatcher<ZstSynchronisableAdaptor>>& ZstSynchronisable::synchronisable_event_dispatcher()
{
	return m_synchronisable_events;
}

void ZstSynchronisable::process_events()
{
	m_synchronisable_events->process_events();
}

void ZstSynchronisable::announce_update()
{
	m_synchronisable_events->defer([this](ZstSynchronisableAdaptor* adaptor) {
		adaptor->on_synchronisable_updated(this);
	});
	m_synchronisable_events->invoke([this](ZstSynchronisableAdaptor* adaptor) {
		adaptor->synchronisable_has_event(this);
	});
}

void ZstSynchronisable::dispatch_destroyed()
{
}

ZstSynchronisableAdaptor* ZstSynchronisable::synchronisable_events()
{
    return m_synchronisable_events->get_default_adaptor().get();
}

void ZstSynchronisable::on_activation()
{
}

void ZstSynchronisable::on_deactivation()
{
}

}
