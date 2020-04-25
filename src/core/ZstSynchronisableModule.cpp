#include "ZstSynchronisableModule.h"

namespace showtime {

ZstSynchronisableModule::ZstSynchronisableModule() : 
	m_synchronisable_events(std::make_shared<ZstEventDispatcher<std::shared_ptr<ZstSynchronisableAdaptor> > >())
{
}

void ZstSynchronisableModule::init_adaptors()
{
    m_synchronisable_events->add_adaptor(ZstSynchronisableModule::downcasted_shared_from_this<ZstSynchronisableModule>());
}

void ZstSynchronisableModule::process_events()
{
    m_synchronisable_events->process_events();
    
    //Reap objects last
    m_reaper.reap_all();

    //The event queue is clear so remove dead IDs
    std::lock_guard<std::mutex> lock(m_mtx);
    m_dead_syncronisable_IDS.clear();
}

void ZstSynchronisableModule::flush_events()
{
    m_synchronisable_events->flush();
}

void ZstSynchronisableModule::add_dead_synchronisable_ID(unsigned int syncronisable_ID)
{
    std::lock_guard<std::mutex> lock(m_mtx);
    m_dead_syncronisable_IDS.insert(syncronisable_ID);
}

bool ZstSynchronisableModule::already_removed_synchronisable(unsigned int syncronisable_ID)
{
    return (m_dead_syncronisable_IDS.find(syncronisable_ID) != m_dead_syncronisable_IDS.end());
}


void ZstSynchronisableModule::synchronisable_has_event(ZstSynchronisable * synchronisable)
{
    auto id = synchronisable->instance_id();
    m_synchronisable_events->defer([this, synchronisable, id](std::shared_ptr<ZstSynchronisableAdaptor> adaptor) {
        // Make sure the synchronisable object hasn't gone away
        if (m_dead_syncronisable_IDS.find(id) == m_dead_syncronisable_IDS.end()) {
            this->synchronisable_process_events(synchronisable);
        }
    });
}

std::shared_ptr < ZstEventDispatcher<std::shared_ptr<ZstSynchronisableAdaptor> > > & ZstSynchronisableModule::synchronisable_events()
{
    return m_synchronisable_events;
}

ZstReaper & ZstSynchronisableModule::reaper() {
	return m_reaper;
}

}
