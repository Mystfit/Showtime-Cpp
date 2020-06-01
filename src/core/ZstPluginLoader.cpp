#include "ZstPluginLoader.h"

namespace showtime {
	std::vector<std::string> ZstPluginLoader::get_plugin_files(std::string& dir)
	{
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

	std::shared_ptr< ZstEventDispatcher< std::shared_ptr< ZstPluginAdaptor > > > & ZstPluginLoader::plugin_events()
	{
		return m_plugin_events;
	}
}