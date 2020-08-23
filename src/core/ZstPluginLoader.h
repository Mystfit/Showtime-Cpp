#pragma once

#include <memory>
#include <unordered_map>
#include <boost/dll.hpp>
#include <showtime/adaptors/ZstPluginAdaptor.hpp>
#include <showtime/ZstPlugin.h>
#include <showtime/ZstFilesystemUtils.h>
#include "ZstEventModule.h"
#include "ZstEventDispatcher.hpp"

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


        // Module overrides
        ZST_EXPORT virtual void init_adaptors() override;
        ZST_EXPORT virtual void process_events() override;
        ZST_EXPORT virtual void flush_events() override;

        //Paths
        ZST_EXPORT void set_plugin_path(const char* path);
        ZST_EXPORT const char* get_plugin_path();
        ZST_EXPORT void set_plugin_data_path(const char* path);
        ZST_EXPORT const char* get_plugin_data_path();

        // Scan and load all plugins in a folder
        ZST_EXPORT void load();

        // Get all loaded plugins
        ZST_EXPORT std::vector<std::shared_ptr<ZstPlugin> > get_plugins();

        // Plugin events
        ZST_EXPORT std::shared_ptr < ZstEventDispatcher<ZstPluginAdaptor> >& plugin_events();
    
    private:
        ZST_EXPORT std::vector<fs::path> plugin_lib_paths(const fs::path& dir);

        std::shared_ptr<ZstEventDispatcher<ZstPluginAdaptor> > m_plugin_events;
        std::unordered_map<std::string, ZstLoadedPlugin> m_loaded_plugins;

        //Paths
        fs::path m_plugin_path;
        fs::path m_plugin_data_path;
    };
}