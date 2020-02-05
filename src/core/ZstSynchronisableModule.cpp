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
}

void ZstSynchronisableModule::flush_events()
{
    m_synchronisable_events->flush();
}

void ZstSynchronisableModule::on_synchronisable_has_event(ZstSynchronisable * synchronisable)
{
    m_synchronisable_events->defer([this, synchronisable](std::shared_ptr<ZstSynchronisableAdaptor> adaptor) {
		this->synchronisable_process_events(synchronisable);
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
