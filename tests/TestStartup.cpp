#define BOOST_TEST_MODULE Library startup and joining

#include "TestCommon.hpp"

using namespace ZstTest;

std::string performer_name = "test_performer";
std::string server_name = "test_server";
std::string server_address = "127.0.0.1:40004";
std::string bad_server_address = "255.255.255.255:1111";

BOOST_AUTO_TEST_CASE(init) {
	auto test_client = std::make_shared<ShowtimeClient>();
	test_client->init("test", true);
	BOOST_TEST(test_client->is_init_completed());
	test_client->destroy();
}

BOOST_AUTO_TEST_CASE(early_library_destroy){
    auto test_client = std::make_shared<ShowtimeClient>();
	test_client->destroy();
	BOOST_TEST(!test_client->is_init_completed());
}

BOOST_AUTO_TEST_CASE(root_performer_exists) {
    auto test_client = std::make_shared<ShowtimeClient>();
	BOOST_TEST(!test_client->get_root());

	test_client->init(performer_name.c_str(), true);
	BOOST_TEST(test_client->get_root());
	test_client->destroy();
}

BOOST_AUTO_TEST_CASE(double_init) {
    auto test_client = std::make_shared<ShowtimeClient>();
	test_client->init(performer_name.c_str(), true);
	test_client->init("wrong_performer", true);
	BOOST_TEST(test_client->get_root()->URI().path() == performer_name.c_str());
	test_client->destroy();
}

BOOST_FIXTURE_TEST_CASE(single_server_connection, FixtureInitAndCreateServer){
	//Testing abort connection if we're already connected
	test_client->join(server_address.c_str());
	test_client->join(server_address.c_str());
	BOOST_TEST(!test_client->is_connecting());
}

BOOST_FIXTURE_TEST_CASE(sync_join, FixtureInitAndCreateServer){
	//Testing sync join by address
	test_client->join(server_address.c_str());
	BOOST_TEST(test_client->is_connected());

	//Testing sync leave
	test_client->leave();
	BOOST_TEST(!test_client->is_connected());

	//Test sync join again to verify we cleaned up properly the first time
	test_client->join(server_address.c_str());
	BOOST_TEST(test_client->is_connected());
}

BOOST_FIXTURE_TEST_CASE(sync_join_by_name, FixtureInitAndCreateServer){
	//Testing joining by name
	WAIT_UNTIL_STAGE_BEACON
	test_client->join_by_name(server_name.c_str());
	BOOST_TEST(test_client->is_connected());
}

BOOST_FIXTURE_TEST_CASE(async_join, FixtureInitAndCreateServer, TEST_TIMEOUT){
	auto connectCallback = std::make_shared< TestConnectionEvents>();
	test_client->add_connection_adaptor(connectCallback);
	test_client->join_async(server_address.c_str());
	wait_for_event(test_client, connectCallback, 2);
	BOOST_TEST(test_client->is_connected());
	BOOST_TEST_REQUIRE(connectCallback->is_synced);
}

BOOST_FIXTURE_TEST_CASE(autojoin_by_name, FixtureInitAndCreateServer){
	//Testing autojoin by name
	test_client->auto_join_by_name(server_name.c_str());
	BOOST_TEST(test_client->is_connected());
}

BOOST_FIXTURE_TEST_CASE(autojoin_by_name_async, FixtureInitAndCreateServer) {
	//Testing autojoin by name
	test_client->auto_join_by_name_async(server_name.c_str());
	WAIT_UNTIL_STAGE_BEACON
	BOOST_TEST(test_client->is_connected());
}

BOOST_FIXTURE_TEST_CASE(async_join_callback_adaptor, FixtureInitAndCreateServer){
	//Test async join
	auto connectCallback = std::make_shared< TestConnectionEvents>();
	test_client->add_connection_adaptor(connectCallback);
	test_client->join_async(server_address.c_str());
	wait_for_event(test_client, connectCallback, 1);
	BOOST_TEST_REQUIRE(test_client->is_connected());
}

BOOST_FIXTURE_TEST_CASE(sync_join_bad_address, FixtureInitAndCreateServer){
	//Testing sync join timeout
	test_client->join(bad_server_address.c_str());
	BOOST_TEST(!test_client->is_connected());
}

BOOST_FIXTURE_TEST_CASE(async_join_bad_address_timeout, FixtureInitAndCreateServer){
	//Test async join timeout
	test_client->join_async(bad_server_address.c_str());
	WAIT_UNTIL_STAGE_TIMEOUT
	BOOST_TEST(!test_client->is_connected());
}

BOOST_FIXTURE_TEST_CASE(double_connection, FixtureInitAndCreateServer){
    //Testing abort connection start if we're already connecting
    test_client->join_async(bad_server_address.c_str());
	BOOST_TEST(test_client->is_connecting());
    test_client->join(bad_server_address.c_str());
	BOOST_TEST(!test_client->is_connected());
    WAIT_UNTIL_STAGE_TIMEOUT
	BOOST_TEST(!test_client->is_connecting());
}

BOOST_FIXTURE_TEST_CASE(list_discovered_servers, FixtureInit){
	auto detected_server_name = std::string("detected_server");
	auto detected_server = std::make_shared< ShowtimeServer>(detected_server_name.c_str(), STAGE_ROUTER_PORT + 10);
	WAIT_UNTIL_STAGE_BEACON
	ZstServerAddressBundle bundle;
	test_client->get_discovered_servers(bundle);
	BOOST_TEST(bundle.size() > 0);
	BOOST_TEST(bundle.item_at(0).name == detected_server_name);
	detected_server->destroy();
}

BOOST_FIXTURE_TEST_CASE(discovered_servers_callback_adaptor, FixtureInit){
	//Create adaptor
	auto discovery_adaptor = std::make_shared<TestConnectionEvents>();
	test_client->add_connection_adaptor(discovery_adaptor);

	//Create a new server for the client to discover
	auto detected_server_name = std::string("detected_server");
	auto detected_server = std::make_shared< ShowtimeServer>(detected_server_name.c_str(), STAGE_ROUTER_PORT + 10);
	wait_for_event(test_client, discovery_adaptor, 1);
	BOOST_TEST(discovery_adaptor->last_discovered_server.name == detected_server_name);
	detected_server->destroy();
}

BOOST_FIXTURE_TEST_CASE(discovered_servers_update, FixtureJoinServer) {
	auto detected_server = std::make_shared< ShowtimeServer>("detected_server", STAGE_ROUTER_PORT + 10);
	WAIT_UNTIL_STAGE_BEACON
	ZstServerAddressBundle bundle;
	test_client->get_discovered_servers(bundle);
	BOOST_TEST(bundle.size() == 2);
	detected_server->destroy();
}

BOOST_FIXTURE_TEST_CASE(root_performer_activate_on_join, FixtureJoinServer)
{   
    auto performer_activated = std::make_shared<TestSynchronisableEvents>();
	test_client->get_root()->add_adaptor(performer_activated);
	BOOST_TEST(performer_activated->num_calls() == 1);
	BOOST_TEST(test_client->get_root()->is_activated());
}
