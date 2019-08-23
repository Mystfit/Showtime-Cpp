#pragma once

#include "ZstExports.h"
#include "ZstReaper.h"
#include "ZstEventDispatcher.hpp"

#include "adaptors/ZstSynchronisableAdaptor.hpp"
#include "liasons/ZstSynchronisableLiason.hpp"

class ZST_EXPORT ZstSynchronisableModule :
    public ZstSynchronisableAdaptor,
    public ZstSynchronisableLiason
{
public:
    ZST_EXPORT ~ZstSynchronisableModule();
	ZST_EXPORT virtual void init();
	ZST_EXPORT virtual void destroy();
    
    ZST_EXPORT virtual void process_events();
    ZST_EXPORT virtual void flush_events();

    ZST_EXPORT virtual void on_synchronisable_has_event(ZstSynchronisable * synchronisable) override;
    ZST_EXPORT ZstEventDispatcher<ZstSynchronisableAdaptor*> & synchronisable_events();
	ZST_EXPORT ZstReaper & reaper();

private:
	ZstReaper m_reaper;
    ZstEventDispatcher<ZstSynchronisableAdaptor*> m_synchronisable_events;
};
