#include "TestCommon.hpp"
#include <boost/thread.hpp>

#define MEM_LEAK_LOOPS 100000;

using namespace ZstTest;

struct BenchmarkEventLoop {
public:
	void operator()() {
		while (1) {
			try {
				boost::this_thread::interruption_point();
				zst_poll_once();
				std::this_thread::sleep_for(std::chrono::milliseconds(0));
			}
			catch (boost::thread_interrupted) {
				ZstLog::net(LogLevel::debug, "Benchmark event loop exiting.");
				break;
			}
		}
	}
};


int test_benchmark(bool reliable, int send_rate, int send_amount)
{
	ZstLog::app(LogLevel::debug, "Creating entities and cables");

	OutputComponent * test_output = new OutputComponent("bench_test_out", reliable);
	InputComponent * test_input = new InputComponent("bench_test_in", 10, false);
	zst_activate_entity(test_output);
	zst_activate_entity(test_input);
	zst_connect_cable(test_input->input(), test_output->output());

	int count = send_amount;

	ZstLog::app(LogLevel::debug, "Sending {} messages", count);
	// Wait until our message tripmeter has received all the messages
	auto delta_time = std::chrono::milliseconds(-1);
	std::chrono::time_point<std::chrono::system_clock> end, last, now;
	auto start = std::chrono::system_clock::now();
	last = start;
	int last_message_count = 0;
	int received_count = 0;
	int delta_messages = 0;
	double mps = 0.0;
	int remaining_messages = count;
	int delta_queue = 0;
	int last_queue_count = 0;
	int last_processed_message_count = 0;
	long queue_speed = 0;
	int num_sent = 0;
	int alert_rate = 2000;
	bool hit = false;
	long long totalmps = 0;
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

	//Cleanup
	delete test_output;
	delete test_input;

	return totalmps / samples;
}


int main(int argc, char **argv)
{
	TestRunner runner("TestBenchmark", argv[0]);

	//Create threaded event loop to handle polling
	boost::thread eventloop_thread = boost::thread(BenchmarkEventLoop());

	ZstLog::app(LogLevel::notification, "Starting reliable benchmark test");
	int mps_reliable = test_benchmark(true, 0, 200000);
	ZstLog::app(LogLevel::notification, "Reliable avg mps: {}", mps_reliable);

	ZstLog::app(LogLevel::notification, "Starting unreliable benchmark test");
	int mps_unreliable = test_benchmark(false, 1, 10000);
	ZstLog::app(LogLevel::notification, "Unreliable avg mps: {}", mps_unreliable);

	eventloop_thread.interrupt();
	eventloop_thread.join();

	return 0;
}
