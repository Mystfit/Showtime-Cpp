#pragma once

#include <memory>
#include <unordered_map>
#include <boost/dll.hpp>

#include "ZstEventModule.h"
#include "ZstEventDispatcher.hpp"
#include "adaptors/ZstPluginAdaptor.hpp"
#include "ZstPlugin.h"
#include "ZstFilesystemUtils.h"

namespace showtime {

    struct ZstLoadedPlugin {
        boost::dll::shared_library library;
        std::shared_ptr<ZstPlugin> plugin;
    };

    class ZST_CLASS_EXPORTED ZstPluginLoader :
        public ZstEventModule
    {
    public:
        ZST_EXPORT ZstPluginLoader();
        ZST_EXPORT ~ZstPluginLoader();

        ZST_EXPORT std::vector<std::shared_ptr<ZstPlugin> > get_plugins();

        ZST_EXPORT virtual void init_adaptors() override;
        ZST_EXPORT virtual void process_events() override;
        ZST_EXPORT virtual void flush_events() override;

        // Scan and load all plugins in a folder
        ZST_EXPORT void load(const fs::path& plugin_dir);

        // Plugin events
        ZST_EXPORT std::shared_ptr < ZstEventDispatcher<std::shared_ptr<ZstPluginAdaptor> > >& plugin_events();
    
    private:
        ZST_EXPORT std::vector<fs::path> plugin_lib_paths(const fs::path& dir);

        std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstPluginAdaptor> > > m_plugin_events;

        std::unordered_map<std::string, ZstLoadedPlugin> m_loaded_plugins;
    };
}