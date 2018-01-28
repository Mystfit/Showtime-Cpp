#include <ZstSynchronisable.h>
#include <algorithm>
#include <assert.h>
#include "ZstINetworkInteractor.h"

ZstSynchronisable::ZstSynchronisable() :
	m_network_interactor(NULL),
	m_sync_status(SyncStatus::DEACTIVATED),
	m_activation_events(NULL),
	m_deactivation_events(NULL),
	m_activation_hook(NULL),
	m_deactivation_hook(NULL)
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
	if (activation_status() == SyncStatus::ACTIVATED) {
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
	if (is_deactivated() || activation_status() != ZstSynchronisable::ACTIVATION_QUEUED)
	{
		set_activation_status(SyncStatus::ACTIVATION_QUEUED);
		m_activation_events->enqueue(this);
		if (m_network_interactor)
			m_network_interactor->enqueue_synchronisable_event(this);
	}
}

void ZstSynchronisable::set_deactivated()
{
	if (is_activated() || activation_status() != ZstSynchronisable::DEACTIVATION_QUEUED)
	{
		set_activation_status(SyncStatus::DEACTIVATION_QUEUED);
		m_deactivation_events->enqueue(this);
		if (m_network_interactor)
			m_network_interactor->enqueue_synchronisable_event(this);
	}
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

ZstINetworkInteractor * ZstSynchronisable::network_interactor()
{
	return m_network_interactor;
}

void ZstSynchronisable::process_events()
{
	m_activation_events->process();
	m_deactivation_events->process();
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
		break;
	case SyncStatus::ACTIVATED:
		break;
	case SyncStatus::DEACTIVATING:
		break;
	case SyncStatus::DEACTIVATION_QUEUED:
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

void ZstActivationEvent::run(ZstSynchronisable * target)
{
	target->set_activation_status(ZstSynchronisable::SyncStatus::ACTIVATED);
	target->on_activated();
}

void ZstDeactivationEvent::run(ZstSynchronisable * target)
{
	target->set_activation_status(ZstSynchronisable::SyncStatus::DEACTIVATED);
	target->on_deactivated();
}
