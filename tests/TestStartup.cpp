#define BOOST_TEST_MODULE Library startup and joining

#include "TestCommon.hpp"

using namespace ZstTest;

std::string performer_name = "test_performer";
std::string server_name = "test_server";
std::string server_address = "127.0.0.1:40004";
std::string bad_server_address = "255.255.255.255:1111";

BOOST_AUTO_TEST_CASE(early_library_destroy){
	zst_destroy();
	BOOST_TEST(!zst_is_init_completed());
}

BOOST_AUTO_TEST_CASE(root_performer_exists) {
	BOOST_TEST(!zst_get_root());

	zst_init(performer_name.c_str(), true);
	BOOST_TEST(zst_get_root());
	zst_destroy();
}

BOOST_AUTO_TEST_CASE(double_init) {
	zst_init(performer_name.c_str(), true);
	zst_init("wrong_performer", true);
	BOOST_TEST(zst_get_root()->URI().path() == performer_name.c_str());
	zst_destroy();
}

BOOST_FIXTURE_TEST_CASE(single_server_connection, FixtureInitAndCreateServer){
	//Testing abort connection if we're already connected
	zst_join(server_address.c_str());
	zst_join(server_address.c_str());
	BOOST_TEST(!zst_is_connecting());
	zst_leave();
}

BOOST_FIXTURE_TEST_CASE(sync_join, FixtureInitAndCreateServer){
	//Testing sync join by address
	zst_join(server_address.c_str());
	BOOST_TEST(zst_is_connected());

	//Testing sync leave
	zst_leave();
	BOOST_TEST(!zst_is_connected());

	//Test sync join again to verify we cleaned up properly the first time
	zst_join(server_address.c_str());
	BOOST_TEST(zst_is_connected());
}

BOOST_FIXTURE_TEST_CASE(sync_join_by_name, FixtureInitAndCreateServer){
	//Testing joining by name
	WAIT_UNTIL_STAGE_BEACON
	zst_join_by_name(server_name.c_str());
	BOOST_TEST(zst_is_connected());
}

BOOST_FIXTURE_TEST_CASE(async_join, FixtureInitAndCreateServer){
	zst_join_async(server_address.c_str());
	TAKE_A_BREATH
	BOOST_TEST(zst_is_connected());
}

BOOST_FIXTURE_TEST_CASE(autojoin_by_name, FixtureInitAndCreateServer){
	//Testing autojoin by name
	zst_auto_join_by_name(server_name.c_str());
	BOOST_TEST(zst_is_connected());
}

BOOST_FIXTURE_TEST_CASE(autojoin_by_name_async, FixtureInitAndCreateServer) {
	//Testing autojoin by name
	zst_auto_join_by_name_async(server_name.c_str());
	WAIT_UNTIL_STAGE_BEACON
	BOOST_TEST(zst_is_connected());
}

BOOST_FIXTURE_TEST_CASE(async_join_callback_adaptor, FixtureInitAndCreateServer){
	//Test async join
	auto connectCallback = std::make_shared< TestConnectionEvents>();
	zst_add_session_adaptor(connectCallback.get());
	zst_join_async(server_address.c_str());
	wait_for_event(connectCallback.get(), 1);
	BOOST_TEST_REQUIRE(zst_is_connected());
}

BOOST_FIXTURE_TEST_CASE(sync_join_bad_address, FixtureInitAndCreateServer){
	//Testing sync join timeout
	zst_join(bad_server_address.c_str());
	BOOST_TEST(!zst_is_connected());
}

BOOST_FIXTURE_TEST_CASE(async_join_bad_address_timeout, FixtureInitAndCreateServer){
	//Test async join timeout
	zst_join_async(bad_server_address.c_str());
	WAIT_UNTIL_STAGE_TIMEOUT
	BOOST_TEST(!zst_is_connected());
}

BOOST_FIXTURE_TEST_CASE(double_connection, FixtureInitAndCreateServer){
    //Testing abort connection start if we're already connecting
    zst_join_async(bad_server_address.c_str());
	BOOST_TEST(zst_is_connecting());
    zst_join(bad_server_address.c_str());
	BOOST_TEST(!zst_is_connected());
    WAIT_UNTIL_STAGE_TIMEOUT
	BOOST_TEST(!zst_is_connecting());
}

BOOST_FIXTURE_TEST_CASE(list_discovered_servers, FixtureInitAndCreateServer){
	WAIT_UNTIL_STAGE_BEACON
	ZstServerAddressBundle bundle;
	zst_get_discovered_servers(bundle);
	BOOST_TEST(bundle.size() > 0);
	BOOST_TEST(bundle.item_at(0).name == server_name);
}

BOOST_FIXTURE_TEST_CASE(discovered_servers_callback_adaptor, FixtureInit){
	//Create adaptor
	auto discovery_adaptor = std::make_shared<TestConnectionEvents>();
	zst_add_session_adaptor(discovery_adaptor.get());

	//Create a new server for the client to discover
	auto detected_server_name = std::string("detected_server");
	auto detected_server = zst_create_server(detected_server_name.c_str(), STAGE_ROUTER_PORT + 10);
	wait_for_event(discovery_adaptor.get(), 1);
	BOOST_TEST(discovery_adaptor->last_discovered_server.name == detected_server_name);

	//Cleanup
	zst_destroy_server(detected_server);
}

BOOST_FIXTURE_TEST_CASE(discovered_servers_update, FixtureJoinServer) {
	auto detected_server = zst_create_server("detected_server", STAGE_ROUTER_PORT + 10);
	WAIT_UNTIL_STAGE_BEACON

	ZstServerAddressBundle bundle;
	zst_get_discovered_servers(bundle);
	BOOST_TEST(bundle.size() == 2);

	//Cleanup
	zst_destroy_server(detected_server);
}

BOOST_FIXTURE_TEST_CASE(root_performer_activate_on_join, FixtureJoinServer)
{   
    auto performer_activated = std::make_shared<TestSynchronisableEvents>();
	zst_get_root()->add_adaptor(performer_activated.get());
	BOOST_TEST(performer_activated->num_calls() == 1);
	BOOST_TEST(zst_get_root()->is_activated());

	//Cleanup
	zst_get_root()->remove_adaptor(performer_activated.get());
}
