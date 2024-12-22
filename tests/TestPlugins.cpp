#include "TestCommon.hpp"
#include <showtime/adaptors/ZstPluginAdaptor.hpp>

#define BOOST_TEST_MODULE Plugins

#ifdef _DEBUG
    #define _CRTDBG_MAP_ALLOC
    #include <stdlib.h>
    #include <crtdbg.h>
#endif

using namespace ZstTest;

struct InitLeakDetection {
    InitLeakDetection() {
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
        //_CrtSetBreakAlloc(213475); // Break on this allocation number
		//_CrtSetBreakAlloc(158132); // Break on this allocation number
    }
};

static InitLeakDetection initLeakDetection;

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

	~FixtureCorePluginFactory() {
		// Ensure factory is deactivated
		if (factory) {
			test_client->deactivate_entity(factory);
		}
	}
};

// Base fixture for math components
struct FixtureCorePluginMathComponent : 
	public FixtureCorePluginFactory
{
protected:
	std::vector<ZstComponent*> created_components;

	ZstComponent* create_math_component(const char* type, const char* name) {
		ZstURI creatable_path_expected = factory->URI() + ZstURI(type);
		ZstURIBundle creatable_bundle;
		factory->get_creatables(&creatable_bundle);
		auto creatable_path = std::find_if(creatable_bundle.begin(), creatable_bundle.end(), [creatable_path_expected](auto it) {
			return it == creatable_path_expected;
		});
		auto component = dynamic_cast<ZstComponent*>(test_client->create_entity(*creatable_path, name));
		if (component) {
			created_components.push_back(component);
		}
		return component;
	}

	~FixtureCorePluginMathComponent() {
		// Cleanup all created components
		for (auto component : created_components) {
			if (component) {
				test_client->deactivate_entity(component);
			}
		}
		created_components.clear();
	}
};

struct FixtureCorePluginAdder : 
	public FixtureCorePluginMathComponent
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
		adder = create_math_component("adder", "test_adder");
		augend = dynamic_cast<ZstInputPlug*>(adder->get_child_by_URI(adder->URI() + ZstURI("augend")));
		addend = dynamic_cast<ZstInputPlug*>(adder->get_child_by_URI(adder->URI() + ZstURI("addend")));
		sum = dynamic_cast<ZstOutputPlug*>(adder->get_child_by_URI(adder->URI() + ZstURI("sum")));
	}
};

struct FixtureCorePluginMultiplier :
	public FixtureCorePluginMathComponent
{
	ZstComponent* multiplier;
	ZstInputPlug* multiplicand;
	ZstInputPlug* multiplier_input;
	ZstOutputPlug* product;

	FixtureCorePluginMultiplier() :
		multiplier(nullptr),
		multiplicand(nullptr),
		multiplier_input(nullptr),
		product(nullptr)
	{
		multiplier = create_math_component("multiplier", "test_multiplier");
		multiplicand = dynamic_cast<ZstInputPlug*>(multiplier->get_child_by_URI(multiplier->URI() + ZstURI("multiplicand")));
		multiplier_input = dynamic_cast<ZstInputPlug*>(multiplier->get_child_by_URI(multiplier->URI() + ZstURI("multiplier")));
		product = dynamic_cast<ZstOutputPlug*>(multiplier->get_child_by_URI(multiplier->URI() + ZstURI("product")));
	}
};

struct FixtureCorePluginSubtractor :
	public FixtureCorePluginMathComponent
{
	ZstComponent* subtractor;
	ZstInputPlug* minuend;
	ZstInputPlug* subtrahend;
	ZstOutputPlug* difference;

	FixtureCorePluginSubtractor() :
		subtractor(nullptr),
		minuend(nullptr),
		subtrahend(nullptr),
		difference(nullptr)
	{
		subtractor = create_math_component("subtractor", "test_subtractor");
		minuend = dynamic_cast<ZstInputPlug*>(subtractor->get_child_by_URI(subtractor->URI() + ZstURI("minuend")));
		subtrahend = dynamic_cast<ZstInputPlug*>(subtractor->get_child_by_URI(subtractor->URI() + ZstURI("subtrahend")));
		difference = dynamic_cast<ZstOutputPlug*>(subtractor->get_child_by_URI(subtractor->URI() + ZstURI("difference")));
	}
};

struct FixtureCorePluginDivider :
	public FixtureCorePluginMathComponent
{
	ZstComponent* divider;
	ZstInputPlug* dividend;
	ZstInputPlug* divisor;
	ZstOutputPlug* quotient;

	FixtureCorePluginDivider() :
		divider(nullptr),
		dividend(nullptr),
		divisor(nullptr),
		quotient(nullptr)
	{
		divider = create_math_component("divider", "test_divider");
		dividend = dynamic_cast<ZstInputPlug*>(divider->get_child_by_URI(divider->URI() + ZstURI("dividend")));
		divisor = dynamic_cast<ZstInputPlug*>(divider->get_child_by_URI(divider->URI() + ZstURI("divisor")));
		quotient = dynamic_cast<ZstOutputPlug*>(divider->get_child_by_URI(divider->URI() + ZstURI("quotient")));
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

	// Cleanup created entity
	test_client->deactivate_entity(adder);
}

BOOST_FIXTURE_TEST_CASE(plugin_adder_ordered, FixtureCorePluginAdder) {
	int current_wait = 0;
	auto push_A = std::make_unique<OutputComponent>("pushA");
	auto push_B = std::make_unique<OutputComponent>("pushB");
	auto sink = std::make_unique<InputComponent>("sink");
	auto push_A_ptr = push_A.get();
	auto push_B_ptr = push_B.get();
	auto sink_ptr = sink.get();
	test_client->get_root()->add_child(push_A_ptr);
	test_client->get_root()->add_child(push_B_ptr);
	test_client->get_root()->add_child(sink_ptr);
	test_client->connect_cable(augend, push_A->output());
	test_client->connect_cable(addend, push_B->output());
	test_client->connect_cable(sink->input(), sum);
	push_A->output()->append_int(2);
	push_B->output()->append_int(5);
	sink->execute_upstream();

	while (sink->num_hits < 1 && ++current_wait < 1000) {
		test_client->poll_once();
	}
	BOOST_TEST(sink->input()->int_at(0) == 7);

	// Cleanup cables and child components
	test_client->destroy_cable(test_client->find_cable(augend->URI(), push_A->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(addend->URI(), push_B->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(sink->input()->URI(), sum->URI()));
	test_client->get_root()->remove_child(push_A_ptr);
	test_client->get_root()->remove_child(push_B_ptr);
	test_client->get_root()->remove_child(sink_ptr);
}

BOOST_FIXTURE_TEST_CASE(plugin_multiplier_basic, FixtureCorePluginMultiplier) {
	int current_wait = 0;
	auto push_A = std::make_unique<OutputComponent>("pushA");
	auto push_B = std::make_unique<OutputComponent>("pushB");
	auto sink = std::make_unique<InputComponent>("sink");
	auto push_A_ptr = push_A.get();
	auto push_B_ptr = push_B.get();
	auto sink_ptr = sink.get();
	test_client->get_root()->add_child(push_A_ptr);
	test_client->get_root()->add_child(push_B_ptr);
	test_client->get_root()->add_child(sink_ptr);
	test_client->connect_cable(multiplicand, push_A->output());
	test_client->connect_cable(multiplier_input, push_B->output());
	test_client->connect_cable(sink->input(), product);
	push_A->output()->append_float(4.0f);
	push_B->output()->append_float(3.0f);
	sink->execute_upstream();

	while (sink->num_hits < 1 && ++current_wait < 1000) {
		test_client->poll_once();
	}
	BOOST_TEST(sink->input()->float_at(0) == 12.0f);

	// Cleanup cables and child components
	test_client->destroy_cable(test_client->find_cable(multiplicand->URI(), push_A->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(multiplier_input->URI(), push_B->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(sink->input()->URI(), product->URI()));
	test_client->get_root()->remove_child(push_A_ptr);
	test_client->get_root()->remove_child(push_B_ptr);
	test_client->get_root()->remove_child(sink_ptr);
}

BOOST_FIXTURE_TEST_CASE(plugin_subtractor_basic, FixtureCorePluginSubtractor) {
	int current_wait = 0;
	auto push_A = std::make_unique<OutputComponent>("pushA");
	auto push_B = std::make_unique<OutputComponent>("pushB");
	auto sink = std::make_unique<InputComponent>("sink");
	auto push_A_ptr = push_A.get();
	auto push_B_ptr = push_B.get();
	auto sink_ptr = sink.get();
	test_client->get_root()->add_child(push_A_ptr);
	test_client->get_root()->add_child(push_B_ptr);
	test_client->get_root()->add_child(sink_ptr);
	test_client->connect_cable(minuend, push_A->output());
	test_client->connect_cable(subtrahend, push_B->output());
	test_client->connect_cable(sink->input(), difference);
	push_A->output()->append_float(10.0f);
	push_B->output()->append_float(3.0f);
	sink->execute_upstream();

	while (sink->num_hits < 1 && ++current_wait < 1000) {
		test_client->poll_once();
	}
	BOOST_TEST(sink->input()->float_at(0) == 7.0f);

	// Cleanup child components
	// Cleanup cables and child components
	test_client->destroy_cable(test_client->find_cable(minuend->URI(), push_A->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(subtrahend->URI(), push_B->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(sink->input()->URI(), difference->URI()));
	test_client->get_root()->remove_child(push_A_ptr);
	test_client->get_root()->remove_child(push_B_ptr);
	test_client->get_root()->remove_child(sink_ptr);
}

BOOST_FIXTURE_TEST_CASE(plugin_divider_basic, FixtureCorePluginDivider) {
	int current_wait = 0;
	auto push_A = std::make_unique<OutputComponent>("pushA");
	auto push_B = std::make_unique<OutputComponent>("pushB");
	auto sink = std::make_unique<InputComponent>("sink");
	auto push_A_ptr = push_A.get();
	auto push_B_ptr = push_B.get();
	auto sink_ptr = sink.get();
	test_client->get_root()->add_child(push_A_ptr);
	test_client->get_root()->add_child(push_B_ptr);
	test_client->get_root()->add_child(sink_ptr);
	test_client->connect_cable(dividend, push_A->output());
	test_client->connect_cable(divisor, push_B->output());
	test_client->connect_cable(sink->input(), quotient);
	push_A->output()->append_float(15.0f);
	push_B->output()->append_float(3.0f);
	sink->execute_upstream();

	while (sink->num_hits < 1 && ++current_wait < 1000) {
		test_client->poll_once();
	}
	BOOST_TEST(sink->input()->float_at(0) == 5.0f);

	// Cleanup child components
	// Cleanup cables and child components
	test_client->destroy_cable(test_client->find_cable(dividend->URI(), push_A->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(divisor->URI(), push_B->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(sink->input()->URI(), quotient->URI()));
	test_client->get_root()->remove_child(push_A_ptr);
	test_client->get_root()->remove_child(push_B_ptr);
	test_client->get_root()->remove_child(sink_ptr);
}

BOOST_FIXTURE_TEST_CASE(plugin_divider_by_zero, FixtureCorePluginDivider) {
	int current_wait = 0;
	auto push_A = std::make_unique<OutputComponent>("pushA");
	auto push_B = std::make_unique<OutputComponent>("pushB");
	auto sink = std::make_unique<InputComponent>("sink");
	auto push_A_ptr = push_A.get();
	auto push_B_ptr = push_B.get();
	auto sink_ptr = sink.get();
	test_client->get_root()->add_child(push_A_ptr);
	test_client->get_root()->add_child(push_B_ptr);
	test_client->get_root()->add_child(sink_ptr);
	test_client->connect_cable(dividend, push_A->output());
	test_client->connect_cable(divisor, push_B->output());
	test_client->connect_cable(sink->input(), quotient);
	push_A->output()->append_float(15.0f);
	push_B->output()->append_float(0.0f);
	sink->execute_upstream();

	while (sink->num_hits < 1 && ++current_wait < 1000) {
		test_client->poll_once();
	}
	BOOST_TEST(sink->input()->float_at(0) == 0.0f);

	// Cleanup child components
	// Cleanup cables and child components
	test_client->destroy_cable(test_client->find_cable(dividend->URI(), push_A->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(divisor->URI(), push_B->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(sink->input()->URI(), quotient->URI()));
	test_client->get_root()->remove_child(push_A_ptr);
	test_client->get_root()->remove_child(push_B_ptr);
	test_client->get_root()->remove_child(sink_ptr);
}

BOOST_FIXTURE_TEST_CASE(plugin_math_list_operations, FixtureCorePluginMathComponent) {
	// Test all math components with lists of different sizes
	int current_wait = 0;
	auto push_A = std::make_unique<OutputComponent>("pushA");
	auto push_B = std::make_unique<OutputComponent>("pushB");
	auto sink = std::make_unique<InputComponent>("sink");
	auto push_A_ptr = push_A.get();
	auto push_B_ptr = push_B.get();
	auto sink_ptr = sink.get();
	test_client->get_root()->add_child(push_A_ptr);
	test_client->get_root()->add_child(push_B_ptr);
	test_client->get_root()->add_child(sink_ptr);

	// Test multiplier with lists
	auto mult = create_math_component("multiplier", "test_mult");
	auto multiplicand = dynamic_cast<ZstInputPlug*>(mult->get_child_by_URI(mult->URI() + ZstURI("multiplicand")));
	auto multiplier_input = dynamic_cast<ZstInputPlug*>(mult->get_child_by_URI(mult->URI() + ZstURI("multiplier")));
	auto product = dynamic_cast<ZstOutputPlug*>(mult->get_child_by_URI(mult->URI() + ZstURI("product")));

	test_client->connect_cable(multiplicand, push_A->output());
	test_client->connect_cable(multiplier_input, push_B->output());
	test_client->connect_cable(sink->input(), product);

	push_A->output()->append_float(2.0f);
	push_A->output()->append_float(3.0f);
	push_B->output()->append_float(4.0f);
	sink->execute_upstream();

	while (sink->num_hits < 1 && ++current_wait < 1000) {
		test_client->poll_once();
	}
	BOOST_TEST(sink->input()->float_at(0) == 8.0f);
	BOOST_TEST(sink->input()->float_at(1) == 3.0f);

	// Cleanup child components
	// Cleanup cables and child components
	test_client->destroy_cable(test_client->find_cable(multiplicand->URI(), push_A->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(multiplier_input->URI(), push_B->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(sink->input()->URI(), product->URI()));
	test_client->get_root()->remove_child(push_A_ptr);
	test_client->get_root()->remove_child(push_B_ptr);
	test_client->get_root()->remove_child(sink_ptr);
}

BOOST_FIXTURE_TEST_CASE(plugin_math_negative_numbers, FixtureCorePluginMathComponent) {
	int current_wait = 0;
	auto push_A = std::make_unique<OutputComponent>("pushA");
	auto push_B = std::make_unique<OutputComponent>("pushB");
	auto sink = std::make_unique<InputComponent>("sink");
	auto push_A_ptr = push_A.get();
	auto push_B_ptr = push_B.get();
	auto sink_ptr = sink.get();
	test_client->get_root()->add_child(push_A_ptr);
	test_client->get_root()->add_child(push_B_ptr);
	test_client->get_root()->add_child(sink_ptr);

	// Test subtractor with negative numbers
	auto sub = create_math_component("subtractor", "test_sub");
	auto minuend = dynamic_cast<ZstInputPlug*>(sub->get_child_by_URI(sub->URI() + ZstURI("minuend")));
	auto subtrahend = dynamic_cast<ZstInputPlug*>(sub->get_child_by_URI(sub->URI() + ZstURI("subtrahend")));
	auto difference = dynamic_cast<ZstOutputPlug*>(sub->get_child_by_URI(sub->URI() + ZstURI("difference")));

	test_client->connect_cable(minuend, push_A->output());
	test_client->connect_cable(subtrahend, push_B->output());
	test_client->connect_cable(sink->input(), difference);

	push_A->output()->append_float(-5.0f);
	push_B->output()->append_float(3.0f);
	sink->execute_upstream();

	while (sink->num_hits < 1 && ++current_wait < 1000) {
		test_client->poll_once();
	}
	BOOST_TEST(sink->input()->float_at(0) == -8.0f);

	// Test multiplier with negative numbers
	auto mult = create_math_component("multiplier", "test_mult");
	auto multiplicand = dynamic_cast<ZstInputPlug*>(mult->get_child_by_URI(mult->URI() + ZstURI("multiplicand")));
	auto multiplier_input = dynamic_cast<ZstInputPlug*>(mult->get_child_by_URI(mult->URI() + ZstURI("multiplier")));
	auto product = dynamic_cast<ZstOutputPlug*>(mult->get_child_by_URI(mult->URI() + ZstURI("product")));

	test_client->connect_cable(multiplicand, push_A->output());
	test_client->connect_cable(multiplier_input, push_B->output());
	test_client->connect_cable(sink->input(), product);

	push_A->output()->append_float(-2.0f);
	push_B->output()->append_float(-3.0f);
	sink->execute_upstream();

	while (sink->num_hits < 2 && ++current_wait < 1000) {
		test_client->poll_once();
	}
	BOOST_TEST(sink->input()->float_at(0) == 6.0f);

	// Cleanup child components
	// Cleanup cables and child components
	test_client->destroy_cable(test_client->find_cable(minuend->URI(), push_A->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(subtrahend->URI(), push_B->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(sink->input()->URI(), difference->URI()));
	test_client->destroy_cable(test_client->find_cable(multiplicand->URI(), push_A->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(multiplier_input->URI(), push_B->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(sink->input()->URI(), product->URI()));
	test_client->get_root()->remove_child(push_A_ptr);
	test_client->get_root()->remove_child(push_B_ptr);
	test_client->get_root()->remove_child(sink_ptr);
}

BOOST_FIXTURE_TEST_CASE(plugin_math_list_operations_extended, FixtureCorePluginMathComponent) {
	int current_wait = 0;
	auto push_A = std::make_unique<OutputComponent>("pushA");
	auto push_B = std::make_unique<OutputComponent>("pushB");
	auto sink = std::make_unique<InputComponent>("sink");
	auto push_A_ptr = push_A.get();
	auto push_B_ptr = push_B.get();
	auto sink_ptr = sink.get();
	test_client->get_root()->add_child(push_A_ptr);
	test_client->get_root()->add_child(push_B_ptr);
	test_client->get_root()->add_child(sink_ptr);

	// Test subtractor with lists
	auto sub = create_math_component("subtractor", "test_sub");
	auto minuend = dynamic_cast<ZstInputPlug*>(sub->get_child_by_URI(sub->URI() + ZstURI("minuend")));
	auto subtrahend = dynamic_cast<ZstInputPlug*>(sub->get_child_by_URI(sub->URI() + ZstURI("subtrahend")));
	auto difference = dynamic_cast<ZstOutputPlug*>(sub->get_child_by_URI(sub->URI() + ZstURI("difference")));

	test_client->connect_cable(minuend, push_A->output());
	test_client->connect_cable(subtrahend, push_B->output());
	test_client->connect_cable(sink->input(), difference);

	push_A->output()->append_float(10.0f);
	push_A->output()->append_float(20.0f);
	push_B->output()->append_float(3.0f);
	sink->execute_upstream();

	while (sink->num_hits < 1 && ++current_wait < 1000) {
		test_client->poll_once();
	}
	BOOST_TEST(sink->input()->float_at(0) == 7.0f);
	BOOST_TEST(sink->input()->float_at(1) == 20.0f);

	// Test divider with lists
	auto div = create_math_component("divider", "test_div");
	auto dividend = dynamic_cast<ZstInputPlug*>(div->get_child_by_URI(div->URI() + ZstURI("dividend")));
	auto divisor = dynamic_cast<ZstInputPlug*>(div->get_child_by_URI(div->URI() + ZstURI("divisor")));
	auto quotient = dynamic_cast<ZstOutputPlug*>(div->get_child_by_URI(div->URI() + ZstURI("quotient")));

	test_client->connect_cable(dividend, push_A->output());
	test_client->connect_cable(divisor, push_B->output());
	test_client->connect_cable(sink->input(), quotient);

	push_A->output()->append_float(12.0f);
	push_A->output()->append_float(6.0f);
	push_B->output()->append_float(3.0f);
	sink->execute_upstream();

	while (sink->num_hits < 2 && ++current_wait < 1000) {
		test_client->poll_once();
	}
	BOOST_TEST(sink->input()->float_at(0) == 4.0f);
	BOOST_TEST(sink->input()->float_at(1) == 6.0f);

	// Cleanup child components
	// Cleanup cables and child components
	test_client->destroy_cable(test_client->find_cable(minuend->URI(), push_A->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(subtrahend->URI(), push_B->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(sink->input()->URI(), difference->URI()));
	test_client->destroy_cable(test_client->find_cable(dividend->URI(), push_A->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(divisor->URI(), push_B->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(sink->input()->URI(), quotient->URI()));
	test_client->get_root()->remove_child(push_A_ptr);
	test_client->get_root()->remove_child(push_B_ptr);
	test_client->get_root()->remove_child(sink_ptr);
}

BOOST_FIXTURE_TEST_CASE(plugin_math_edge_cases, FixtureCorePluginMathComponent) {
	int current_wait = 0;
	auto push_A = std::make_unique<OutputComponent>("pushA");
	auto push_B = std::make_unique<OutputComponent>("pushB");
	auto sink = std::make_unique<InputComponent>("sink");
	auto push_A_ptr = push_A.get();
	auto push_B_ptr = push_B.get();
	auto sink_ptr = sink.get();
	test_client->get_root()->add_child(push_A_ptr);
	test_client->get_root()->add_child(push_B_ptr);
	test_client->get_root()->add_child(sink_ptr);

	// Test divider with very small numbers
	auto div = create_math_component("divider", "test_div");
	auto dividend = dynamic_cast<ZstInputPlug*>(div->get_child_by_URI(div->URI() + ZstURI("dividend")));
	auto divisor = dynamic_cast<ZstInputPlug*>(div->get_child_by_URI(div->URI() + ZstURI("divisor")));
	auto quotient = dynamic_cast<ZstOutputPlug*>(div->get_child_by_URI(div->URI() + ZstURI("quotient")));

	test_client->connect_cable(dividend, push_A->output());
	test_client->connect_cable(divisor, push_B->output());
	test_client->connect_cable(sink->input(), quotient);

	push_A->output()->append_float(0.0001f);
	push_B->output()->append_float(0.0001f);
	sink->execute_upstream();

	while (sink->num_hits < 1 && ++current_wait < 1000) {
		test_client->poll_once();
	}
	BOOST_TEST(sink->input()->float_at(0) == 1.0f);

	// Test multiplier with zero
	auto mult = create_math_component("multiplier", "test_mult");
	auto multiplicand = dynamic_cast<ZstInputPlug*>(mult->get_child_by_URI(mult->URI() + ZstURI("multiplicand")));
	auto multiplier_input = dynamic_cast<ZstInputPlug*>(mult->get_child_by_URI(mult->URI() + ZstURI("multiplier")));
	auto product = dynamic_cast<ZstOutputPlug*>(mult->get_child_by_URI(mult->URI() + ZstURI("product")));

	test_client->connect_cable(multiplicand, push_A->output());
	test_client->connect_cable(multiplier_input, push_B->output());
	test_client->connect_cable(sink->input(), product);

	push_A->output()->append_float(5.0f);
	push_B->output()->append_float(0.0f);
	sink->execute_upstream();

	while (sink->num_hits < 2 && ++current_wait < 1000) {
		test_client->poll_once();
	}
	BOOST_TEST(sink->input()->float_at(0) == 0.0f);

	// Cleanup child components
	// Cleanup cables and child components
	test_client->destroy_cable(test_client->find_cable(dividend->URI(), push_A->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(divisor->URI(), push_B->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(sink->input()->URI(), quotient->URI()));
	test_client->destroy_cable(test_client->find_cable(multiplicand->URI(), push_A->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(multiplier_input->URI(), push_B->output()->URI()));
	test_client->destroy_cable(test_client->find_cable(sink->input()->URI(), product->URI()));
	test_client->get_root()->remove_child(push_A_ptr);
	test_client->get_root()->remove_child(push_B_ptr);
	test_client->get_root()->remove_child(sink_ptr);
}
