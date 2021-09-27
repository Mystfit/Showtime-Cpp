#include "TestCommon.hpp"

#define BOOST_TEST_MODULE Library startup

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

BOOST_AUTO_TEST_CASE(client_destruction_cleanup) {
	{
		auto test_client = std::make_shared<ShowtimeClient>();
		test_client->init(performer_name.c_str(), true);
		test_client->log_events()->formatted_log_record() += [](const char* record) {std::cout << record << std::endl; };
		//test_client->log_events()->log_record() += [](const Log::Record* record) {std::cout << record->message << std::endl; };
		test_client->destroy();
	}
}

BOOST_AUTO_TEST_CASE(log_events) {
	
	auto log_events = std::make_shared <TestLogEvents>();
	auto test_client = std::make_shared<ShowtimeClient>();
	test_client->add_log_adaptor(log_events);

	test_client->init(performer_name.c_str(), true);
	log_events->reset_num_calls();
	std::string message = "testmessage";
	Log::app(Log::Level::debug, message.c_str());
	wait_for_event(test_client, log_events, 1);

	/*auto log_record = std::find_if(log_events->records.begin(), log_events->records.end(), [&message](const Log::Record& record) {
		return message == record.message;
	});*/
	auto log_record = std::find_if(log_events->formatted_records.begin(), log_events->formatted_records.end(), [&message](const std::string& record) {
		return record.find(message) != record.npos;
	});

	auto record_found = (log_record != log_events->formatted_records.end());
	BOOST_TEST(record_found);
	record_found = false;
	//BOOST_TEST(log_record->channel == "app");
	//BOOST_TEST(log_record->level == Log::Level::debug);

	// Make sure we can reattach log events after destroy() was called
	test_client->remove_log_adaptor(log_events);
	test_client->destroy();
	log_events->reset_num_calls();

	std::string second_message = "test2";
	test_client->init(performer_name.c_str(), true);
	test_client->add_log_adaptor(log_events);
	Log::app(Log::Level::debug, second_message.c_str());
	wait_for_event(test_client, log_events, 1);

	/*log_record = std::find_if(log_events->records.begin(), log_events->records.end(), [&second_message](const Log::Record& record) {
		return second_message == record.message;
	});*/
	log_record = std::find_if(log_events->formatted_records.begin(), log_events->formatted_records.end(), [&second_message](const std::string& record) {
		return record.find(second_message) != record.npos;
	});

	record_found = (log_record != log_events->formatted_records.end());
	BOOST_TEST(record_found);

	test_client->destroy();
}

BOOST_FIXTURE_TEST_CASE(auto_join_timeout, FixtureInit) {
	//Testing abort connection if we're already connected
	Log::app(Log::Level::debug, "before autojoin");
	test_client->auto_join_by_name("fakeserver");
	BOOST_TEST(!test_client->is_connecting());

	Log::app(Log::Level::debug, "before asyncautojoin");
	test_client->auto_join_by_name_async("fakeserver");
	WAIT_UNTIL_STAGE_TIMEOUT
	Log::app(Log::Level::debug, "before asyncautojoin check");
	BOOST_TEST(!test_client->is_connecting());
}

BOOST_FIXTURE_TEST_CASE(single_server_connection, FixtureInitAndCreateServerWithEpheremalPort){
	//Testing abort connection if we're already connected
	test_client->join(server_address.c_str());
	test_client->join(server_address.c_str());
	BOOST_TEST(!test_client->is_connecting());
}

BOOST_FIXTURE_TEST_CASE(sync_join, FixtureInit){
	auto test_server = std::make_unique<ShowtimeServer>();
	test_server->init("sync_join", STAGE_ROUTER_PORT);
	auto server_address = fmt::format("127.0.0.1:{}", STAGE_ROUTER_PORT);
	//Testing sync join by address
	test_client->join(server_address.c_str());
	BOOST_TEST(test_client->is_connected());

	//Testing sync leave
	test_client->leave();
	BOOST_TEST(!test_client->is_connected());

	// TODO: Reconnecting immediately will indefinitely hang the program
	TAKE_A_BREATH

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
	wait_for_event(test_client, connectCallback, 1);
	BOOST_TEST(test_client->is_connected());
	BOOST_TEST_REQUIRE(connectCallback->is_connected);
}

BOOST_FIXTURE_TEST_CASE(async_join_event, FixtureInitAndCreateServerWithEpheremalPort, TEST_TIMEOUT) {
	auto connectCallback = std::make_shared< TestConnectionEvents>();
	test_client->add_connection_adaptor(connectCallback);
	bool connected = false;
	test_client->connection_events()->connected_to_server() += [&connected](ShowtimeClient* client, const ZstServerAddress* server) { 
		connected = true; 
	};
	
	test_client->join_async(server_address.c_str());
	wait_for_event(test_client, connectCallback, 1);
	BOOST_TEST(connected);
}

BOOST_FIXTURE_TEST_CASE(autojoin_by_name, FixtureInitAndCreateServerWithEpheremalPort){
	//Testing autojoin by name
	test_client->auto_join_by_name(server_name.c_str());
	BOOST_TEST(test_client->is_connected());
}

BOOST_FIXTURE_TEST_CASE(server_beacon_lost, FixtureInitAndCreateServerWithEpheremalPort) {
	auto connectCallback = std::make_shared<TestConnectionEvents>();
	test_client->add_connection_adaptor(connectCallback);
	//wait_for_event(test_client, connectCallback, 1);
	auto server_address = test_client->get_discovered_server(server_name.c_str());
	connectCallback->reset_num_calls();

	test_server->destroy();
	WAIT_UNTIL_STAGE_TIMEOUT
	wait_for_event(test_client, connectCallback, 1);
	auto lost_server = (connectCallback->lost_servers.end() != std::find_if(
		connectCallback->lost_servers.begin(),
		connectCallback->lost_servers.end(),
		[&server_address](const ZstServerAddress& server) {
			return server.name == server_address.name;
		}
	));
	BOOST_TEST(lost_server);
}

BOOST_FIXTURE_TEST_CASE(leave_server_event, FixtureJoinServer) {
	auto connectCallback = std::make_shared<TestConnectionEvents>();
	test_client->add_connection_adaptor(connectCallback);
	connectCallback->reset_num_calls();

	test_client->leave();
	wait_for_event(test_client, connectCallback, 1);
	BOOST_TEST(!connectCallback->is_connected);
}


BOOST_FIXTURE_TEST_CASE(server_shutdown_disconnects_client, FixtureJoinServer) {
	auto connectCallback = std::make_shared<TestConnectionEvents>();
	test_client->add_connection_adaptor(connectCallback);
	connectCallback->reset_num_calls();

	test_server->destroy();
	wait_for_event(test_client, connectCallback, 1);
	
	BOOST_TEST(!connectCallback->is_connected);
	BOOST_TEST(!test_client->is_connected());
}

BOOST_FIXTURE_TEST_CASE(autojoin_by_name_async, FixtureInit) {
	//Testing autojoin by name
	std::string delayed_server_name = "delayed_server";
	test_client->auto_join_by_name_async(delayed_server_name.c_str());
	WAIT_UNTIL_STAGE_BEACON
	auto delayed_server = std::make_unique<ShowtimeServer>();
	delayed_server->init(delayed_server_name.c_str());
	WAIT_UNTIL_STAGE_BEACON
	BOOST_TEST(test_client->is_connected());
	delayed_server->destroy();
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
	Log::app(Log::Level::notification, "Make sure that we didn't block");
	WAIT_UNTIL_STAGE_TIMEOUT
	BOOST_TEST(!test_client->is_connected());
}

BOOST_FIXTURE_TEST_CASE(double_connection, FixtureInitAndCreateServerWithEpheremalPort){
    //Testing abort connection start if we're already connecting
    test_client->join_async(bad_server_address.c_str());
	BOOST_TEST(test_client->is_connecting());
    test_client->join(bad_server_address.c_str());
	BOOST_TEST(!test_client->is_connected());
}

BOOST_FIXTURE_TEST_CASE(list_discovered_servers, FixtureInit) {
	ZstServerAddress server_address{"detected_server", ""
};
	auto detected_server = std::make_shared< ShowtimeServer>();
	detected_server->init(server_address.name.c_str());
	WAIT_UNTIL_STAGE_BEACON
	ZstServerAddressBundle bundle;
	test_client->get_discovered_servers(&bundle);

	auto found_server = (bundle.end() != std::find_if(bundle.begin(), bundle.end(), [&server_address](const ZstServerAddress& server) {
		return server.name == server_address.name;
	}));
	BOOST_TEST(found_server);
	detected_server->destroy();
}

BOOST_FIXTURE_TEST_CASE(discovered_servers_callback_adaptor, FixtureInit){
	ZstServerAddress server_address{ "detected_server", "" };

	//Create adaptor
	auto discovery_adaptor = std::make_shared<TestConnectionEvents>();
	test_client->add_connection_adaptor(discovery_adaptor);

	//Create a new server for the client to discover
	auto detected_server = std::make_shared< ShowtimeServer>();
	detected_server->init(server_address.name.c_str());
	WAIT_UNTIL_STAGE_BEACON
	wait_for_event(test_client, discovery_adaptor, 1);
	
	auto found_server = (discovery_adaptor->discovered_servers.end() != std::find_if(discovery_adaptor->discovered_servers.begin(), discovery_adaptor->discovered_servers.end(), [&server_address](const ZstServerAddress& server) {
		return server.name == server_address.name;
	}));
	BOOST_TEST_REQUIRE(found_server);
	detected_server->destroy();
}

BOOST_FIXTURE_TEST_CASE(discovered_servers_update, FixtureInitAndCreateServerWithEpheremalPort) {
	ZstServerAddress server_address{ "detected_server", "" };
	auto detected_server = std::make_shared< ShowtimeServer>();
	detected_server->init(server_address.name.c_str());
	WAIT_UNTIL_STAGE_BEACON
	ZstServerAddressBundle bundle;
	test_client->get_discovered_servers(&bundle);
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


BOOST_FIXTURE_TEST_CASE(blocking_poll, FixtureInitAndCreateServerWithEpheremalPort)
{
	// Pump leftover events first
	for (int i = 0; i < 20; ++i) {
		test_client->poll_once();
	}

	test_client->join_async(server_address.c_str());
	test_client->poll_once(true);
	BOOST_TEST_REQUIRE(test_client->is_connected());
}
