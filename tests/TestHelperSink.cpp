#include <iostream>
#include <chrono>
#include <thread>

#include "TestCommon.hpp"

using namespace ZstTest;
using namespace showtime;

std::shared_ptr<ShowtimeClient> test_client;

int main(int argc,char **argv){

	ZstLog::app(LogLevel::notification, "In sink process");

	bool force_launch = false;
	if (!force_launch) {
		bool skip = true;
		for (int i = 0; i < argc; ++i) {
			if (strcmp(argv[i], "test") == 0) {
				skip = false;
			}
		}
		if (skip) {
			ZstLog::app(LogLevel::warn, "Skipping, 'test' command line flag not set");
			return 0;
		}
	}

	test_client = std::make_shared<ShowtimeClient>();
	test_client->init("TestHelperSink", true);
    test_client->auto_join_by_name(TEST_SERVER_NAME);

	Sink * sink = new Sink("sink_ent");
	test_client->get_root()->add_child(sink);
	
	while (sink->last_received_code >= 0){
		test_client->poll_once();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
    
	ZstLog::app(LogLevel::notification, "Sink is leaving");
	test_client->destroy();
	return 0;
}
