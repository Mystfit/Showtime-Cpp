#include "ZstSynchronisableModule.h"

void ZstSynchronisableModule::init()
{
    m_synchronisable_events.add_adaptor(this);
}

void ZstSynchronisableModule::destroy()
{
    m_synchronisable_events.flush();
    m_synchronisable_events.remove_all_adaptors();
}

void ZstSynchronisableModule::process_events()
{
    m_synchronisable_events.process_events();
    
    //Reap objects last
    m_reaper.reap_all();
}

void ZstSynchronisableModule::flush_events()
{
    m_synchronisable_events.flush();
}

void ZstSynchronisableModule::on_synchronisable_has_event(ZstSynchronisable * synchronisable)
{
    m_synchronisable_events.defer([this, synchronisable](ZstSynchronisableAdaptor * dlg) {
        this->synchronisable_process_events(synchronisable);
    });
}

ZstEventDispatcher<ZstSynchronisableAdaptor*> & ZstSynchronisableModule::synchronisable_events()
{
    return m_synchronisable_events;
}

ZstReaper & ZstSynchronisableModule::reaper() {
	return m_reaper;
}
