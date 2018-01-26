#include <ZstSynchronisable.h>
#include <algorithm>
#include <assert.h>
#include "ZstINetworkInteractor.h"

ZstSynchronisable::ZstSynchronisable() :
	m_network_interactor(NULL),
	m_sync_status(SyncStatus::DEACTIVATED)
{
}

ZstSynchronisable::ZstSynchronisable(const ZstSynchronisable & other)
{
	m_sync_status = other.m_sync_status;
}

void ZstSynchronisable::attach_activation_event(ZstSynchronisableEvent * event)
{
	m_activation_events.push_back(event);
	
	//If we're already activated we can trigger the callback immediately
	if (activation_status() == SyncStatus::ACTIVATED) {
		event->cast_run(this);
	}
}

void ZstSynchronisable::attach_deactivation_event(ZstSynchronisableEvent * event)
{
	m_deactivation_events.push_back(event);
}

void ZstSynchronisable::detach_activation_event(ZstSynchronisableEvent * event)
{
	m_activation_events.erase(std::remove(m_activation_events.begin(), m_activation_events.end(), event), m_activation_events.end());
}

void ZstSynchronisable::detach_deactivation_event(ZstSynchronisableEvent * event)
{
	m_deactivation_events.erase(std::remove(m_deactivation_events.begin(), m_deactivation_events.end(), event), m_deactivation_events.end());
}

void ZstSynchronisable::set_activated()
{
	set_activation_status(SyncStatus::ACTIVATION_QUEUED);
}

void ZstSynchronisable::set_deactivated()
{
	set_activation_status(SyncStatus::DEACTIVATION_QUEUED);
}

bool ZstSynchronisable::is_activated()
{
	return m_sync_status == SyncStatus::ACTIVATED;
}

bool ZstSynchronisable::is_deactivated()
{
	return m_sync_status == SyncStatus::DEACTIVATED;
}

ZstSynchronisable::SyncStatus ZstSynchronisable::activation_status()
{
	return m_sync_status;
}

void ZstSynchronisable::set_network_interactor(ZstINetworkInteractor * network_interactor)
{
	m_network_interactor = network_interactor;
}

void ZstSynchronisable::process_events()
{
	switch (m_sync_status)
	{
	case SyncStatus::ACTIVATION_QUEUED:
		on_activated();
		for (auto c : m_activation_events) {
			c->cast_run(this);
		}
		set_activation_status(SyncStatus::ACTIVATED);
		break;
	case SyncStatus::DEACTIVATION_QUEUED:
		on_deactivated();
		for (auto c : m_deactivation_events) {
			c->cast_run(this);
		}
		set_activation_status(SyncStatus::DEACTIVATED);
		break;
	default:
		break;
	}
}

void ZstSynchronisable::set_activation_status(SyncStatus status)
{
	switch (status) 
	{
	case SyncStatus::DEACTIVATED:
		break;
	case SyncStatus::ACTIVATING:
		break;
	case SyncStatus::ACTIVATION_QUEUED:
		if (m_network_interactor)
			m_network_interactor->queue_synchronisable_event(this);
		break;
	case SyncStatus::ACTIVATED:
		break;
	case SyncStatus::DEACTIVATING:
		break;
	case SyncStatus::DEACTIVATION_QUEUED:
		if (m_network_interactor)
			m_network_interactor->queue_synchronisable_event(this);
		break;
	case SyncStatus::ERR_PERFORMER_NOT_FOUND:
		break;
	case SyncStatus::ERR_PARENT_NOT_FOUND:
		break;
	case SyncStatus::ERR_ENTITY_ALREADY_EXISTS:
		break;
	default:
		throw std::range_error("Did not understand status");
		break;
	}

	m_sync_status = status;
}
