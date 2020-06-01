#pragma once

#include <memory>
#include <unordered_map>
#include "ZstEventModule.h"
#include "ZstEventDispatcher.hpp"
#include "adaptors/ZstPluginAdaptor.hpp"
#include "ZstPlugin.h"

namespace showtime {

    class ZST_CLASS_EXPORTED ZstPluginLoader :
        public ZstEventModule
    {
    public:
        ZST_EXPORT virtual std::vector<std::string> get_plugin_files(std::string& dir);

        ZST_EXPORT virtual void init_adaptors() override;
        ZST_EXPORT virtual void process_events() override;
        ZST_EXPORT virtual void flush_events() override;

        ZST_EXPORT std::shared_ptr < ZstEventDispatcher<std::shared_ptr<ZstPluginAdaptor> > >& plugin_events();
    
    private:
        std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstPluginAdaptor> > > m_plugin_events;

        std::unordered_map<std::string, ZstPlugin*> m_loaded_plugins;
    };
}