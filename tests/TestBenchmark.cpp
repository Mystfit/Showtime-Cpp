#include "TestCommon.hpp"
#include <boost/thread.hpp>

#define MEM_LEAK_LOOPS 100000;

using namespace ZstTest;

long double test_benchmark(std::string& server_name, std::shared_ptr<ShowtimeClient> client, bool reliable, int send_rate, int send_amount)
{

	ZstLog::app(LogLevel::debug, "Creating entities and cables");

	// Create listeners
	auto hierarchy_events = std::make_shared<TestEntityEvents>();
	client->add_hierarchy_adaptor(hierarchy_events);
	hierarchy_events->reset_num_calls();

	// Create remote input
	auto remote_client = std::make_shared<ShowtimeClient>();
	remote_client->init("remote", true);
	auto remote_eventloop = FixtureEventLoop(remote_client);
	remote_client->auto_join_by_name(server_name.c_str());
	BOOST_TEST_REQUIRE(remote_client->is_connected());

	auto test_input = std::make_unique<InputComponent>("bench_test_in", 10, false);
	remote_client->get_root()->add_child(test_input.get());
	BOOST_TEST_REQUIRE(test_input->input()->is_activated());
	wait_for_event(client, hierarchy_events, 1);
	auto proxy_input_plug = dynamic_cast<ZstInputPlug*>(client->find_entity(test_input->input()->URI()));

	// Create local output
	auto test_output = std::make_unique<OutputComponent>("bench_test_out", reliable);
	client->get_root()->add_child(test_output.get());

	auto cable = client->connect_cable(proxy_input_plug, test_output->output());
	BOOST_TEST_REQUIRE(cable);

	int count = send_amount;

	ZstLog::app(LogLevel::debug, "Sending {} messages", count);
	// Wait until our message tripmeter has received all the messages
	auto delta_time = std::chrono::milliseconds(-1);
	std::chrono::time_point<std::chrono::system_clock> last, now;
	auto start = std::chrono::system_clock::now();
	last = start;
	int last_message_count = 0;
	int received_count = 0;
	int delta_messages = 0;
	long double mps = 0.0;
	int remaining_messages = count;
	int last_queue_count = 0;
	int last_processed_message_count = 0;
	int num_sent = 0;
	int alert_rate = 2000;
	bool hit = false;
	long double totalmps = 0;
	int samples = 0;

	for (int i = 0; i < count; ++i) {
		test_output->send(i);
		num_sent++;
		std::this_thread::sleep_for(std::chrono::milliseconds(send_rate));

		if (num_sent % alert_rate == 0) {
			ZstLog::app(LogLevel::debug, "Sent {} messages. Remaining: {}. Last received val: {}", num_sent, count - num_sent, test_input->last_received_val);
		}

		if (test_input->num_hits > 0 && test_input->num_hits % alert_rate == 0) {
			//Display progress
			received_count = test_input->num_hits;

			now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
			delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - last);
			delta_messages = received_count - last_message_count;
			if (delta_messages == 0 || delta_time.count() == 0)
				continue;

			last = now;
			mps = (long)delta_messages / ((long)delta_time.count() / 1000.0);
			if (std::isnan(mps))
				continue;

			remaining_messages = count - received_count;
			last_message_count = received_count;
			last_queue_count = num_sent - received_count;
			hit = true;
			totalmps += mps;
			samples++;
			ZstLog::app(LogLevel::debug, "Received {} messages p/s over period : {} ms", mps, (delta_time.count()));
		}

		if (hit && test_input->num_hits % alert_rate != 0) {
			hit = false;
		}
	}

	ZstLog::app(LogLevel::debug, "Sent all messages. Waiting for recv");

	int max_loops_idle = 10000;
	int num_loops_idle = 0;

	do {	
		received_count = test_input->num_hits;
		remaining_messages = count - received_count;
		
		//Break out of the loop if we lost some messages
		if (last_processed_message_count == received_count) {
			num_loops_idle++;
		}
		else {
			num_loops_idle = 0;
		}
		last_processed_message_count = received_count;
		if (!reliable && num_loops_idle > max_loops_idle) {
			break;
		}

		if (test_input->num_hits > 0 && test_input->num_hits % alert_rate == 0) {
			//Display progress
			now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
			delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - last);
			delta_messages = received_count - last_message_count;
			if (delta_messages == 0 || delta_time.count() == 0)
				continue;
			if (std::isnan(mps))
				continue;
			
			last = now;
			mps = (long)delta_messages / ((long)delta_time.count() / 1000.0);
			if (mps < 0)
				mps = 0;
			last_message_count = received_count;
			hit = true;
			ZstLog::app(LogLevel::debug, "Received {} messages p/s over period: {} ms", mps, (delta_time.count()));
			totalmps += mps;
			samples++;
		}

		if (hit && test_input->num_hits % alert_rate != 0) {
			hit = false;
		}


	} while(test_input->num_hits < count);

	if (reliable) {
		assert(test_input->num_hits == count);
		ZstLog::app(LogLevel::debug, "Received all messages. Sent: {}, Received:{}", count, test_input->num_hits);
	}
	else {
		ZstLog::app(LogLevel::debug, "Unreliable messages. Sent: {}, Received: {}, Lost: {}", count, test_input->num_hits, count - test_input->num_hits);
	}

	auto mps_avg = totalmps / samples;
	ZstLog::app(LogLevel::notification, "Reliable avg mps: {}", mps_avg);

	return mps_avg;
}


BOOST_FIXTURE_TEST_CASE(benchmark_reliable, FixtureJoinEventLoop) {

	BOOST_CHECK_NO_THROW(test_benchmark(server_name, test_client, true, 0, 200000));
}

#ifdef ZST_BUILD_DRAFT_API
BOOST_FIXTURE_TEST_CASE(benchmark_reliable, FixtureJoinEventLoop) {
	//Create threaded event loop to handle polling
	boost::thread eventloop_thread = boost::thread(BenchmarkEventLoop(test_client));
	BOOST_CHECK_NO_THROW(test_benchmark(server_name, test_client, false, 1, 10000));
}
#endif
