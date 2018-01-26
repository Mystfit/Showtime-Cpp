#include <ZstSynchronisable.h>
#include <algorithm>
#include <assert.h>
#include "ZstINetworkInteractor.h"

ZstSynchronisable::ZstSynchronisable() :
	m_activation_queued(false),
	m_deactivation_queued(false),
	m_is_activated(false),
	m_network_interactor(NULL)
{
}

ZstSynchronisable::ZstSynchronisable(const ZstSynchronisable & other)
{
	m_is_activated = other.m_is_activated;
}

void ZstSynchronisable::attach_activation_event(ZstSynchronisableEvent * event)
{
	m_activation_events.push_back(event);
	
	//If we're already activated we can trigger the callback immediately
	if (is_activated()) {
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

bool ZstSynchronisable::is_activated()
{
	return m_is_activated;
}

void ZstSynchronisable::set_network_interactor(ZstINetworkInteractor * network_interactor)
{
	m_network_interactor = network_interactor;
}

void ZstSynchronisable::process_events()
{
	if (m_activation_queued) {
		on_activated();
		for (auto c : m_activation_events) {
			c->cast_run(this);
		}
		m_activation_queued = false;
	} 
	
	if (m_deactivation_queued) {
		on_deactivated();
		for (auto c : m_deactivation_events) {
			c->cast_run(this);
		}
		m_deactivation_queued = false;
	}
}

void ZstSynchronisable::set_activated()
{
	m_is_activated = true;
	m_activation_queued = true;
	if(m_network_interactor)
		m_network_interactor->queue_synchronisable_activation(this);
}

void ZstSynchronisable::set_deactivated()
{
	m_is_activated = false;
	m_deactivation_queued = true;
	if(m_network_interactor)
		m_network_interactor->queue_synchronisable_deactivation(this);
}