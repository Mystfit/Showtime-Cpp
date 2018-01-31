#include <ZstSynchronisable.h>
#include <algorithm>
#include <assert.h>

#include "ZstEventDispatcher.h"
#include "ZstINetworkInteractor.h"

ZstSynchronisable::ZstSynchronisable() :
	m_network_interactor(NULL),
	m_sync_status(ZstSyncStatus::DEACTIVATED),
    m_sync_error(ZstSyncError::NO_ERR)
{
	m_activation_events = new ZstEventDispatcher();
	m_deactivation_events = new ZstEventDispatcher();
	m_activation_hook = new ZstActivationEvent();
	m_deactivation_hook = new ZstDeactivationEvent();
	m_activation_events->attach_pre_event_callback(m_activation_hook);
	m_deactivation_events->attach_post_event_callback(m_deactivation_hook);
}

ZstSynchronisable::~ZstSynchronisable()
{
	delete m_activation_events;
	delete m_deactivation_events;
	delete m_activation_hook;
	delete m_deactivation_hook;
}

ZstSynchronisable::ZstSynchronisable(const ZstSynchronisable & other) : ZstSynchronisable()
{
	m_sync_status = other.m_sync_status;
}

void ZstSynchronisable::attach_activation_event(ZstSynchronisableEvent * event)
{
	m_activation_events->attach_event_listener(event);
	
	//If we're already activated we can trigger the callback immediately
	if (activation_status() == ZstSyncStatus::ACTIVATED) {
		event->cast_run(this);
	}
}

void ZstSynchronisable::attach_deactivation_event(ZstSynchronisableEvent * event)
{
	m_deactivation_events->attach_event_listener(event);
}

void ZstSynchronisable::detach_activation_event(ZstSynchronisableEvent * event)
{
	m_activation_events->remove_event_listener(event);
}

void ZstSynchronisable::detach_deactivation_event(ZstSynchronisableEvent * event)
{
	m_deactivation_events->remove_event_listener(event);
}

void ZstSynchronisable::set_activated()
{
	if (is_deactivated() || activation_status() != ZstSyncStatus::ACTIVATION_QUEUED)
	{
		set_activation_status(ZstSyncStatus::ACTIVATION_QUEUED);
		m_activation_events->enqueue(this);
		if (m_network_interactor)
			m_network_interactor->enqueue_synchronisable_event(this);
	}
}

void ZstSynchronisable::set_activating()
{
	set_activation_status(ZstSyncStatus::ACTIVATING);
}

void ZstSynchronisable::set_deactivated()
{
	if (is_activated() || activation_status() != ZstSyncStatus::DEACTIVATION_QUEUED)
	{
		set_activation_status(ZstSyncStatus::DEACTIVATION_QUEUED);
		m_deactivation_events->enqueue(this);
		if (m_network_interactor)
			m_network_interactor->enqueue_synchronisable_event(this);
	}
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

void ZstSynchronisable::set_network_interactor(ZstINetworkInteractor * network_interactor)
{
	m_network_interactor = network_interactor;
}

ZstINetworkInteractor * ZstSynchronisable::network_interactor()
{
	return m_network_interactor;
}

void ZstSynchronisable::process_events()
{
	m_activation_events->process();
	m_deactivation_events->process();
}

void ZstSynchronisable::set_activation_status(ZstSyncStatus status)
{
	switch (status) 
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
		throw std::range_error("Did not understand status");
		break;
	}

	m_sync_status = status;
}
