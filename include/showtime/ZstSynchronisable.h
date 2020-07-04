#pragma once

#include <memory>
#include <mutex>

#include <showtime/ZstExports.h>
#include <showtime/ZstConstants.h>
#include "adaptors/ZstSynchronisableAdaptor.hpp"

namespace showtime {

    
//Forwards
template<typename T>
class ZstEventDispatcher;

    
class ZST_CLASS_EXPORTED ZstSynchronisable
{
	friend class ZstSynchronisableLiason;
    friend class ZstClient;
public:
	ZST_EXPORT ZstSynchronisable();
	ZST_EXPORT ZstSynchronisable(const ZstSynchronisable & other);
    ZST_EXPORT virtual ~ZstSynchronisable();
    
    ZST_EXPORT virtual void add_adaptor(std::shared_ptr<ZstSynchronisableAdaptor> adaptor);
    ZST_EXPORT virtual void remove_adaptor(std::shared_ptr<ZstSynchronisableAdaptor> adaptor);
	ZST_EXPORT ZstSynchronisableAdaptor* synchronisable_events();

    ZST_EXPORT virtual void on_activation();
    ZST_EXPORT virtual void on_deactivation();

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
	ZST_EXPORT virtual void set_deactivated();
    ZST_EXPORT virtual void set_activation_status(ZstSyncStatus status);
    ZST_EXPORT void set_deactivating();
    ZST_EXPORT void set_error(ZstSyncError e);
	ZST_EXPORT void set_destroyed();
	ZST_EXPORT virtual void set_proxy();
	ZST_EXPORT std::shared_ptr<ZstEventDispatcher<ZstSynchronisableAdaptor> >& synchronisable_event_dispatcher();
	ZST_EXPORT virtual void process_events();
	ZST_EXPORT void announce_update();
	ZST_EXPORT virtual void dispatch_destroyed();

private:
	bool m_is_destroyed;
	ZstSyncStatus m_sync_status;
	ZstSyncError m_sync_error;
	bool m_is_proxy;
    std::shared_ptr< ZstEventDispatcher<ZstSynchronisableAdaptor> > m_synchronisable_events;
	unsigned int m_instance_id;
	static unsigned int s_instance_id_counter;
    std::mutex m_sync_lock;
};

}
