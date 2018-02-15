#pragma once

#include <ZstExports.h>
#include <ZstEvents.h>

class ZstINetworkInteractor;
class ZstEventDispatcher;

enum ZstSyncStatus {
	DEACTIVATED = 0,
	ACTIVATING,
	ACTIVATION_QUEUED,
	ACTIVATED,
	DEACTIVATING,
	DEACTIVATION_QUEUED,
	ERR
};

enum ZstSyncError {
	NO_ERR,
	PERFORMER_NOT_FOUND,
	PARENT_NOT_FOUND,
	ENTITY_ALREADY_EXISTS
};

class ZstSynchronisable {
	friend class ZstActivationEvent;
	friend class ZstDeactivationEvent;
    friend class ZstClient;
public:
	ZST_EXPORT ZstSynchronisable();
	ZST_EXPORT ZstSynchronisable(const ZstSynchronisable & other);
    ZST_EXPORT virtual ~ZstSynchronisable();
    
	ZST_EXPORT void attach_activation_event(ZstSynchronisableEvent * event);
	ZST_EXPORT void attach_deactivation_event(ZstSynchronisableEvent * event);
	ZST_EXPORT void detach_activation_event(ZstSynchronisableEvent * event);
	ZST_EXPORT void detach_deactivation_event(ZstSynchronisableEvent * event);

	ZST_EXPORT void process_events();
    ZST_EXPORT void flush_events();
	ZST_EXPORT virtual void on_activated() = 0;
	ZST_EXPORT virtual void on_deactivated() = 0;

	ZST_EXPORT bool is_activated();
	ZST_EXPORT bool is_deactivated();
	ZST_EXPORT ZstSyncStatus activation_status();
	ZST_EXPORT ZstSyncError last_error();

	//Register graph sender so this entity can comunicate with the graph
	ZST_EXPORT virtual void set_network_interactor(ZstINetworkInteractor * network_interactor);

protected:
	ZST_EXPORT ZstINetworkInteractor * network_interactor();
    ZST_EXPORT virtual void enqueue_activation();
    ZST_EXPORT virtual void enqueue_deactivation();
    ZST_EXPORT virtual void set_activated();
    ZST_EXPORT virtual void set_activating();
    ZST_EXPORT void set_deactivating();
    ZST_EXPORT virtual void set_activation_status(ZstSyncStatus status);
    ZST_EXPORT void set_error(ZstSyncError e);

private:
	ZstEventDispatcher * m_activation_events;
	ZstEventDispatcher * m_deactivation_events;
	ZstActivationEvent * m_activation_hook;
	ZstDeactivationEvent * m_deactivation_hook;

	ZstINetworkInteractor * m_network_interactor;
	ZstSyncStatus m_sync_status;
	ZstSyncError m_sync_error;
};
