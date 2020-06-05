#include "TestCommon.hpp"

#define BOOST_TEST_MODULE Plugins

using namespace ZstTest;

BOOST_FIXTURE_TEST_CASE(load_plugins, FixtureInit) {
	auto loaded_plugins = test_client->plugins();
	bool found = std::find_if(loaded_plugins.begin(), loaded_plugins.end(), [](auto it) {
		return strcmp(it->name(), "core_entities") == 0; 
	}) != loaded_plugins.end();
	BOOST_TEST(found);
}
