#include "TestCommon.hpp"
#include <adaptors/ZstPluginAdaptor.hpp>

#define BOOST_TEST_MODULE Plugins

using namespace ZstTest;

class PluginEvents : 
	public ZstPluginAdaptor,
	public TestAdaptor
{
public:
	std::string last_loaded_plugin;
	std::string last_unloaded_plugin;

	void on_plugin_loaded(std::shared_ptr<ZstPlugin> plugin) override {
		ZstLog::app(LogLevel::notification, "PLUGIN_LOADED: {}", plugin->name());
		inc_calls();
		last_loaded_plugin = plugin->name();
	}

	void on_plugin_unloaded(std::shared_ptr<ZstPlugin> plugin) override {
		ZstLog::app(LogLevel::notification, "PLUGIN_UNLOADED: {}", plugin->name());
		inc_calls();
		last_unloaded_plugin = plugin->name();
	}
};


BOOST_FIXTURE_TEST_CASE(load_plugins, FixtureInit) {
	auto loaded_plugins = test_client->plugins();
	bool found = std::find_if(loaded_plugins.begin(), loaded_plugins.end(), [](auto it) {
		return strcmp(it->name(), "core_entities") == 0; 
	}) != loaded_plugins.end();
	BOOST_TEST(found);
}

BOOST_FIXTURE_TEST_CASE(load_plugins_events, FixtureInit) {
	auto plugin_events = std::make_shared<PluginEvents>();
	test_client->add_plugin_adaptor(plugin_events);
	wait_for_event(test_client, plugin_events, 1);
	BOOST_TEST("core_entities" == plugin_events->last_loaded_plugin);
}
