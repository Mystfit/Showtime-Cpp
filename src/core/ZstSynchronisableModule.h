#pragma once

#include <memory>
#include <mutex>
#include "ZstExports.h"
#include "ZstReaper.h"
#include "ZstEventDispatcher.hpp"

#include "adaptors/ZstSynchronisableAdaptor.hpp"
#include "liasons/ZstSynchronisableLiason.hpp"

namespace showtime {

class ZST_CLASS_EXPORTED ZstSynchronisableModule :
    public ZstSynchronisableAdaptor,
    public ZstSynchronisableLiason
{
public:
	ZST_EXPORT ZstSynchronisableModule();
    ZST_EXPORT ~ZstSynchronisableModule() {};
    ZST_EXPORT virtual void init_adaptors();
    
    ZST_EXPORT virtual void process_events();
    ZST_EXPORT virtual void flush_events();

    ZST_EXPORT void add_dead_synchronisable_ID(unsigned int syncronisable_ID);
    ZST_EXPORT bool already_removed_synchronisable(unsigned int syncronisable_ID);
    ZST_EXPORT virtual void synchronisable_has_event(ZstSynchronisable * synchronisable) override;
    ZST_EXPORT std::shared_ptr < ZstEventDispatcher<std::shared_ptr<ZstSynchronisableAdaptor> > > & synchronisable_events();
	ZST_EXPORT ZstReaper & reaper();

private:
	ZstReaper m_reaper;
    std::set<unsigned int> m_dead_syncronisable_IDS;
    std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstSynchronisableAdaptor> > > m_synchronisable_events;
    std::mutex m_mtx;
};

}
