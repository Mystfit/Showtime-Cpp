#include "TestCommon.hpp"
#include <showtime/adaptors/ZstPluginAdaptor.hpp>

#define BOOST_TEST_MODULE Plugins

using namespace ZstTest;

class PluginEvents : 
	public ZstPluginAdaptor,
	public TestAdaptor
{
public:
	std::string last_loaded_plugin;
	std::string last_unloaded_plugin;

	void on_plugin_loaded(ZstPlugin* plugin) override {
		Log::app(Log::Level::notification, "PLUGIN_LOADED: {}", plugin->name());
		inc_calls();
		last_loaded_plugin = plugin->name();
	}

	void on_plugin_unloaded(ZstPlugin* plugin) override {
		Log::app(Log::Level::notification, "PLUGIN_UNLOADED: {}", plugin->name());
		inc_calls();
		last_unloaded_plugin = plugin->name();
	}
};

struct FixtureCorePlugin : public FixtureJoinServer {
	std::shared_ptr<ZstPlugin> plugin;

	FixtureCorePlugin() {
		auto loaded_plugins = test_client->plugins();
		plugin = *std::find_if(loaded_plugins.begin(), loaded_plugins.end(), [](auto it) {
			return strcmp(it->name(), "core_entities") == 0;
		});
	}
};


struct FixtureCorePluginFactory : public FixtureCorePlugin {
	ZstEntityFactory* factory;

	FixtureCorePluginFactory() {
		ZstURI factory_path_expected = test_client->get_root()->URI() + ZstURI("math_entities");
		ZstEntityFactoryBundle bundle;
		plugin->get_factories(bundle);
		factory = *std::find_if(bundle.begin(), bundle.end(), [factory_path_expected](auto it) {
			return it->URI() == factory_path_expected;
		});
	}
};

struct FixtureCorePluginAdder : 
	public FixtureCorePluginFactory
{
	ZstComponent* adder;
	ZstInputPlug* augend;
	ZstInputPlug* addend;
	ZstOutputPlug* sum;

	FixtureCorePluginAdder() :
		adder(nullptr),
		augend(nullptr),
		addend(nullptr),
		sum(nullptr)
	{
		ZstURI creatable_path_expected = factory->URI() + ZstURI("adder");
		ZstURIBundle creatable_bundle;
		factory->get_creatables(&creatable_bundle);
		auto creatable_path = std::find_if(creatable_bundle.begin(), creatable_bundle.end(), [creatable_path_expected](auto it) {
			return it == creatable_path_expected;
		});
		adder = dynamic_cast<ZstComponent*>(test_client->create_entity(*creatable_path, "test_adder"));
		augend = dynamic_cast<ZstInputPlug*>(adder->get_child_by_URI(adder->URI() + ZstURI("augend")));
		addend = dynamic_cast<ZstInputPlug*>(adder->get_child_by_URI(adder->URI() + ZstURI("addend")));
		sum = dynamic_cast<ZstOutputPlug*>(adder->get_child_by_URI(adder->URI() + ZstURI("sum")));
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
	// TODO: Plugins are already loaded. Need to reload plugins to trigger the event.
}

BOOST_AUTO_TEST_CASE(set_plugin_dir) {
	auto client = std::make_unique<ShowtimeClient>();
	auto plugin_path = fs::path(boost::unit_test::framework::master_test_suite().argv[0]).parent_path().append("plugins");
	client->set_plugin_path(plugin_path.string().c_str());
	client->init("plugin_loader", true);

	auto loaded_plugins = client->plugins();
	bool found = std::find_if(loaded_plugins.begin(), loaded_plugins.end(), [](auto it) {
		return strcmp(it->name(), "core_entities") == 0;
		}) != loaded_plugins.end();
	BOOST_TEST(found);
	client->destroy();
}

BOOST_FIXTURE_TEST_CASE(plugin_factories, FixtureCorePlugin) {
	ZstEntityFactoryBundle bundle;
	ZstURI factory_path = test_client->get_root()->URI() + ZstURI("math_entities");
	plugin->get_factories(bundle);

	BOOST_TEST(bundle.size() > 0);
	bool found = std::find_if(bundle.begin(), bundle.end(), [factory_path](auto it) {
		return it->URI() == factory_path;
	}) != bundle.end();
	BOOST_TEST(found);
}

BOOST_FIXTURE_TEST_CASE(plugin_create_entity, FixtureCorePluginFactory) {
	ZstURI creatable_path_expected = factory->URI() + ZstURI("adder");
	ZstURIBundle creatable_bundle;
	factory->get_creatables(&creatable_bundle);
	auto creatable_path = std::find_if(creatable_bundle.begin(), creatable_bundle.end(), [creatable_path_expected](auto it) {
		return it == creatable_path_expected;
	});
	bool found = (creatable_path != creatable_bundle.end());
	BOOST_TEST(found);

	// Create entity
	auto adder = test_client->create_entity(*creatable_path, "test_adder");
	BOOST_REQUIRE(adder);
}

BOOST_FIXTURE_TEST_CASE(plugin_adder_ordered, FixtureCorePluginAdder) {
	int current_wait = 0;
	auto push_A = std::make_unique<OutputComponent>("pushA");
	auto push_B = std::make_unique<OutputComponent>("pushB");
	auto sink = std::make_unique<InputComponent>("sink");
	test_client->get_root()->add_child(push_A.get());
	test_client->get_root()->add_child(push_B.get());
	test_client->get_root()->add_child(sink.get());
	test_client->connect_cable(augend, push_A->output());
	test_client->connect_cable(addend, push_B->output());
	test_client->connect_cable(sink->input(), sum);
	push_A->output()->append_int(2);
	push_B->output()->append_int(5);
	push_A->execute();

	while (sink->num_hits < 1 && ++current_wait < 1000) {
		test_client->poll_once();
	}
	BOOST_TEST(sink->input()->int_at(0) == 7);
}