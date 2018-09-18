#pragma once

#include <ZstExports.h>
#include <ZstConstants.h>
#include <adaptors/ZstSynchronisableAdaptor.hpp>

//Forwards
class ZstINetworkInteractor;

template<typename T>
class ZstEventDispatcher;

class ZstSynchronisable
{
	friend class ZstSynchronisableLiason;

public:
	ZST_EXPORT ZstSynchronisable();
	ZST_EXPORT ZstSynchronisable(const ZstSynchronisable & other);
    ZST_EXPORT virtual ~ZstSynchronisable();
    
    ZST_EXPORT virtual void add_adaptor(ZstSynchronisableAdaptor * adaptor);
    ZST_EXPORT virtual void remove_adaptor(ZstSynchronisableAdaptor * adaptor);
	ZST_EXPORT ZstEventDispatcher<ZstSynchronisableAdaptor*> * synchronisable_events();

    ZST_EXPORT virtual void on_activation(){};
    ZST_EXPORT virtual void on_deactivation(){};

	ZST_EXPORT bool is_activated();
	ZST_EXPORT bool is_deactivated();
	ZST_EXPORT ZstSyncStatus activation_status();
	ZST_EXPORT ZstSyncError last_error();
	ZST_EXPORT bool is_destroyed();
	ZST_EXPORT bool is_proxy();
	ZST_EXPORT unsigned int instance_id();

protected:
    ZST_EXPORT virtual void enqueue_activation();
    ZST_EXPORT virtual void enqueue_deactivation();
    ZST_EXPORT virtual void set_activated();
    ZST_EXPORT virtual void set_activating();
    ZST_EXPORT virtual void set_activation_status(ZstSyncStatus status);
    ZST_EXPORT void set_deactivating();
    ZST_EXPORT void set_error(ZstSyncError e);
	ZST_EXPORT void set_destroyed();
	ZST_EXPORT virtual void set_proxy();
	ZST_EXPORT void process_events();
	ZST_EXPORT void announce_update();
	ZST_EXPORT virtual void dispatch_destroyed();

private:
	bool m_is_destroyed;
	ZstSyncStatus m_sync_status;
	ZstSyncError m_sync_error;
	bool m_is_proxy;
	ZstEventDispatcher<ZstSynchronisableAdaptor*> * m_synchronisable_events;
	unsigned int m_instance_id;
	static unsigned int s_instance_id_counter;
};
