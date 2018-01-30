#pragma once

#include <ZstExports.h>
#include <ZstEvents.h>

class ZstINetworkInteractor;
class ZstEventDispatcher;

class ZstSynchronisable {
	friend class ZstActivationEvent;
	friend class ZstDeactivationEvent;
public:
	enum SyncStatus {
		DEACTIVATED = 0,
		ACTIVATING,
		ACTIVATION_QUEUED,
		ACTIVATED,
		DEACTIVATING,
		DEACTIVATION_QUEUED,
		ERR_PERFORMER_NOT_FOUND,
		ERR_PARENT_NOT_FOUND,
		ERR_ENTITY_ALREADY_EXISTS
	};

	ZST_EXPORT ZstSynchronisable();
	ZST_EXPORT ~ZstSynchronisable();
	ZST_EXPORT ZstSynchronisable(const ZstSynchronisable & other);
	ZST_EXPORT virtual void attach_activation_event(ZstSynchronisableEvent * event);
	ZST_EXPORT virtual void attach_deactivation_event(ZstSynchronisableEvent * event);
	ZST_EXPORT virtual void detach_activation_event(ZstSynchronisableEvent * event);
	ZST_EXPORT virtual void detach_deactivation_event(ZstSynchronisableEvent * event);

	ZST_EXPORT virtual void process_events();
	ZST_EXPORT virtual void on_activated() = 0;
	ZST_EXPORT virtual void on_deactivated() = 0;

	ZST_EXPORT virtual void set_activated();
	ZST_EXPORT virtual void set_deactivated();
	ZST_EXPORT bool is_activated();
	ZST_EXPORT bool is_deactivated();
	ZST_EXPORT SyncStatus activation_status();

	//Register graph sender so this entity can comunicate with the graph
	ZST_EXPORT virtual void set_network_interactor(ZstINetworkInteractor * network_interactor);

protected:
	ZST_EXPORT virtual void set_activation_status(SyncStatus status);
	ZST_EXPORT ZstINetworkInteractor * network_interactor();

private:
	ZstEventDispatcher * m_activation_events;
	ZstEventDispatcher * m_deactivation_events;
	ZstActivationEvent * m_activation_hook;
	ZstDeactivationEvent * m_deactivation_hook;

	ZstINetworkInteractor * m_network_interactor;
	SyncStatus m_sync_status;
};
