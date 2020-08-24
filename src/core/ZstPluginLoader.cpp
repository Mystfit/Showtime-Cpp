#include "ZstPluginLoader.h"
#include <showtime/ZstFilesystemUtils.h>

namespace showtime {
	ZstPluginLoader::ZstPluginLoader() : 
		m_plugin_events(std::make_shared<ZstEventDispatcher<ZstPluginAdaptor> >())
	{
	}

	ZstPluginLoader::~ZstPluginLoader()
	{
		m_plugin_events->flush_events();
		for (auto plugin : m_loaded_plugins) {
			plugin.second.library.unload();
		}
		m_loaded_plugins.clear();
	}

	std::vector<fs::path> ZstPluginLoader::plugin_lib_paths(const fs::path& dir)
	{
		std::vector<fs::path> plugin_files;

		if (!fs::exists(dir)) {
			Log::net(Log::Level::error, "Plugin path invalid: {}", dir.string());
			return plugin_files;
		}

		for (auto file : fs::directory_iterator(dir)) {
			if (!fs::is_regular_file(file))
				continue;

			if (file.path().extension() != boost::dll::shared_library::suffix())
				continue;

			plugin_files.push_back(file.path());
		}

		return plugin_files;
	}

	std::vector<std::shared_ptr<ZstPlugin>> ZstPluginLoader::get_plugins()
	{
		auto plugins = std::vector < std::shared_ptr<ZstPlugin> >();
		for (auto plugin : m_loaded_plugins) {
			plugins.push_back(plugin.second.plugin);
		}
		return plugins;
	}

	void ZstPluginLoader::init_adaptors()
	{
	}

	void ZstPluginLoader::process_events()
	{
		m_plugin_events->process_events();
	}

	void ZstPluginLoader::flush_events()
	{
	}

	void ZstPluginLoader::set_plugin_path(const char* path)
	{
		m_plugin_path = path;
	}

	const char* ZstPluginLoader::get_plugin_path()
	{
		return m_plugin_path.string().c_str();
	}

	void ZstPluginLoader::set_plugin_data_path(const char* path)
	{
		m_plugin_data_path = path;
	}

	const char* ZstPluginLoader::get_plugin_data_path()
	{
		return m_plugin_data_path.string().c_str();
	}

	void ZstPluginLoader::load()
	{
		if (m_plugin_path.empty()) {
			m_plugin_path = fs::path(boost::dll::program_location().string()).parent_path().append("plugins");
		}

		auto plugins = plugin_lib_paths(m_plugin_path);

		for (auto file : plugins) {
			boost::dll::shared_library lib;
			boost::dll::fs::error_code ec;

			// Strip extension - let boost pick the decorators
			file.replace_extension("");

			// Load plugin library
			lib.load(file.string(), ec, boost::dll::load_mode::append_decorations);
			if (ec.value() != 0) {
				Log::net(Log::Level::error, "Plugin {} load error: {}", file.filename().string(), ec.message());
				continue;
			}

			// Create instance of plugin from the shared lib
			auto plugin_creator = lib.get_alias<ZstPlugin_create_t>("create_plugin");
			std::shared_ptr<ZstPlugin> plugin = plugin_creator();
			plugin->init(m_plugin_data_path.string().c_str());
			
			// Hold onto lib and plugin instances
			m_loaded_plugins[plugin->name()] = ZstLoadedPlugin{lib, plugin};

			// Defer plugin loaded event
			m_plugin_events->defer([plugin](ZstPluginAdaptor* adp) {
				adp->on_plugin_loaded(plugin.get());
			});
			Log::net(Log::Level::debug, "Loaded plugin {} {}.{}.{}", plugin->name(), plugin->version_major(), plugin->version_minor(), plugin->version_patch());
		}
	}

	std::shared_ptr< ZstEventDispatcher<ZstPluginAdaptor > > & ZstPluginLoader::plugin_events()
	{
		return m_plugin_events;
	}
}