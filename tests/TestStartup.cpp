#define BOOST_TEST_MODULE Library startup and joining

#include <boost/test/framework.hpp>
#include "TestCommon.hpp"
#include "fmt/format.h"

using namespace ZstTest;

std::string performer_name = "test_performer";
std::string server_name = "test_server";
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

BOOST_FIXTURE_TEST_CASE(single_server_connection, FixtureInitAndCreateServerWithEpheremalPort){
	//Testing abort connection if we're already connected
	test_client->join(server_address.c_str());
	test_client->join(server_address.c_str());
	BOOST_TEST(!test_client->is_connecting());
}

BOOST_FIXTURE_TEST_CASE(sync_join, FixtureInitAndCreateServerWithEpheremalPort){
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

BOOST_FIXTURE_TEST_CASE(sync_join_by_name, FixtureInitAndCreateServerWithEpheremalPort){
	//Testing joining by name
	WAIT_UNTIL_STAGE_BEACON
	test_client->join_by_name(server_name.c_str());
	BOOST_TEST(test_client->is_connected());
}

BOOST_FIXTURE_TEST_CASE(async_join, FixtureInitAndCreateServerWithEpheremalPort, TEST_TIMEOUT){
	auto connectCallback = std::make_shared< TestConnectionEvents>();
	test_client->add_connection_adaptor(connectCallback);
	test_client->join_async(server_address.c_str());
	wait_for_event(test_client, connectCallback, 2);
	BOOST_TEST(test_client->is_connected());
	BOOST_TEST_REQUIRE(connectCallback->is_synced);
}

BOOST_FIXTURE_TEST_CASE(autojoin_by_name, FixtureInitAndCreateServerWithEpheremalPort){
	//Testing autojoin by name
	test_client->auto_join_by_name(server_name.c_str());
	BOOST_TEST(test_client->is_connected());
}

BOOST_FIXTURE_TEST_CASE(server_stopped_events, FixtureInitAndCreateServerWithEpheremalPort) {
	test_client->auto_join_by_name(server_name.c_str());
	auto server_address = test_client->get_discovered_server(server_name.c_str());

	auto connectCallback = std::make_shared<TestConnectionEvents>();
	test_client->add_connection_adaptor(connectCallback);
	test_server->destroy();
	wait_for_event(test_client, connectCallback, 2);
	auto lost_server = (connectCallback->lost_servers.end() != std::find_if(
		connectCallback->lost_servers.begin(),
		connectCallback->lost_servers.end(),
		[&server_address](const ZstServerAddress& server) {
			return server.name == server_address.name;
		}
	));
	BOOST_TEST(lost_server);
	BOOST_TEST(!connectCallback->is_connected);
	BOOST_TEST(!test_client->is_connected());
}

BOOST_FIXTURE_TEST_CASE(autojoin_by_name_async, FixtureInitAndCreateServerWithEpheremalPort) {
	//Testing autojoin by name
	test_client->auto_join_by_name_async(server_name.c_str());
	WAIT_UNTIL_STAGE_BEACON
	BOOST_TEST(test_client->is_connected());
}

BOOST_FIXTURE_TEST_CASE(async_join_callback_adaptor, FixtureInitAndCreateServerWithEpheremalPort){
	//Test async join
	auto connectCallback = std::make_shared< TestConnectionEvents>();
	test_client->add_connection_adaptor(connectCallback);
	test_client->join_async(server_address.c_str());
	wait_for_event(test_client, connectCallback, 1);
	BOOST_TEST_REQUIRE(test_client->is_connected());
}

BOOST_FIXTURE_TEST_CASE(sync_join_bad_address, FixtureInit){
	//Testing sync join timeout
	test_client->join(bad_server_address.c_str());
	BOOST_TEST(!test_client->is_connected());
}

BOOST_FIXTURE_TEST_CASE(async_join_bad_address_timeout, FixtureInit){
	//Test async join timeout
	test_client->join_async(bad_server_address.c_str());
	WAIT_UNTIL_STAGE_TIMEOUT
	BOOST_TEST(!test_client->is_connected());
}

BOOST_FIXTURE_TEST_CASE(double_connection, FixtureInitAndCreateServerWithEpheremalPort){
    //Testing abort connection start if we're already connecting
    test_client->join_async(bad_server_address.c_str());
	BOOST_TEST(test_client->is_connecting());
    test_client->join(bad_server_address.c_str());
	BOOST_TEST(!test_client->is_connected());
    WAIT_UNTIL_STAGE_TIMEOUT
	BOOST_TEST(!test_client->is_connecting());
}

BOOST_FIXTURE_TEST_CASE(list_discovered_servers, FixtureInit){
	ZstServerAddress server_address("detected_server" , "");
	auto detected_server = std::make_shared< ShowtimeServer>(server_address.name);
	WAIT_UNTIL_STAGE_BEACON
	ZstServerAddressBundle bundle;
	test_client->get_discovered_servers(bundle);

	auto found_server = (bundle.end() != std::find_if(bundle.begin(), bundle.end(), [&server_address](const ZstServerAddress& server) {
		return server.name == server_address.name;
	}));
	BOOST_TEST(found_server);
	detected_server->destroy();
}

BOOST_FIXTURE_TEST_CASE(discovered_servers_callback_adaptor, FixtureInit){
	ZstServerAddress server_address("detected_server", "");

	//Create adaptor
	auto discovery_adaptor = std::make_shared<TestConnectionEvents>();
	test_client->add_connection_adaptor(discovery_adaptor);

	//Create a new server for the client to discover
	auto detected_server = std::make_shared< ShowtimeServer>(server_address.name);
	WAIT_UNTIL_STAGE_BEACON
	wait_for_event(test_client, discovery_adaptor, 1);
	
	auto found_server = (discovery_adaptor->discovered_servers.end() != std::find_if(discovery_adaptor->discovered_servers.begin(), discovery_adaptor->discovered_servers.end(), [&server_address](const ZstServerAddress& server) {
		return server.name == server_address.name;
	}));
	BOOST_TEST_REQUIRE(found_server);
	detected_server->destroy();
}

BOOST_FIXTURE_TEST_CASE(discovered_servers_update, FixtureInitAndCreateServerWithEpheremalPort) {
	ZstServerAddress server_address("detected_server", "");
	auto detected_server = std::make_shared< ShowtimeServer>(server_address.name);
	WAIT_UNTIL_STAGE_BEACON
	ZstServerAddressBundle bundle;
	test_client->get_discovered_servers(bundle);
	auto found_server = (bundle.end() != std::find_if(bundle.begin(), bundle.end(), [&server_address](const ZstServerAddress& server) {
		return server.name == server_address.name;
	}));
	BOOST_TEST(found_server);
	detected_server->destroy();
}

BOOST_FIXTURE_TEST_CASE(root_performer_activate_on_join, FixtureJoinServer)
{   
    auto performer_activated = std::make_shared<TestSynchronisableEvents>();
	test_client->get_root()->add_adaptor(performer_activated);
	BOOST_TEST(performer_activated->num_calls() == 1);
	BOOST_TEST(test_client->get_root()->is_activated());
}
