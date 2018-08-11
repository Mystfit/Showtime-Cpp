#include "TestCommon.hpp"

#define MEM_LEAK_LOOPS 200000;

using namespace ZstTest;

void test_benchmark(bool reliable)
{
	ZstLog::app(LogLevel::debug, "Creating entities and cables");

	OutputComponent * test_output = new OutputComponent("bench_test_out", reliable);
	InputComponent * test_input = new InputComponent("bench_test_in", 10, false);
	zst_activate_entity(test_output);
	zst_activate_entity(test_input);
	zst_connect_cable(test_input->input(), test_output->output());

	int count = MEM_LEAK_LOOPS;

	ZstLog::app(LogLevel::debug, "Sending {} messages", count);
	// Wait until our message tripmeter has received all the messages
	auto delta = std::chrono::milliseconds(-1);
	std::chrono::time_point<std::chrono::system_clock> end, last, now;
	auto start = std::chrono::system_clock::now();
	last = start;
	int last_message_count = 0;
	int message_count = 0;
	int delta_messages = 0;
	double mps = 0.0;
	int remaining_messages = count;
	int queued_messages = 0;
	int delta_queue = 0;
	int last_queue_count = 0;
	long queue_speed = 0;
	int num_sent = 0;

	for (int i = 0; i < count; ++i) {
		test_output->send(10);
		num_sent++;
		zst_poll_once();
		if (test_input->num_hits % 10000 == 0) {
			//Display progress
			message_count = test_input->num_hits;
			queued_messages = num_sent - test_input->num_hits;

			now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
			delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - last);
			delta_messages = message_count - last_message_count;
			delta_queue = queued_messages - last_queue_count;

			last = now;
			mps = (long)delta_messages / (delta.count() / 1000.0);
			queue_speed = static_cast<long>(delta_queue / (delta.count() / 1000.0));

			remaining_messages = count - message_count;
			last_message_count = message_count;
			last_queue_count = queued_messages;

			ZstLog::app(LogLevel::debug, "Processing {} messages per/s. Remaining: {}, Delta T: {} per 10000, Queued: {}, Queuing speed: {} messages per/s", mps, remaining_messages, (delta.count() / 1000.0), queued_messages, queue_speed);
		}
	}

	ZstLog::app(LogLevel::debug, "Sent all messages. Waiting for recv");

	int max_loops_idle = 50;
	int num_loops_idle = 0;

	do {
		zst_poll_once();
		if (test_input->num_hits % 10000 == 0) {
			//Display progress
			message_count = test_input->num_hits;
			now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
			delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - last);
			delta_messages = message_count - last_message_count;
			queued_messages = num_sent - test_input->num_hits;
			last = now;
			mps = (long)delta_messages / (delta.count() / 1000.0);
			remaining_messages = count - message_count;

			//Break out of the loop if we lost some messages
			if (last_message_count == message_count) {
				num_loops_idle++;
				if (num_loops_idle > max_loops_idle)
					break;
			}
			else {
				num_loops_idle = 0;
			}

			last_message_count = message_count;
			ZstLog::app(LogLevel::debug, "Processing {} messages per/s. Remaining: {}, Delta T: {} per 10000, Queued: {}, Queuing speed: {} messages per/s", mps, remaining_messages, (delta.count() / 1000.0), queued_messages, queue_speed);
		}
	} while ((test_input->num_hits < count));

	assert(test_input->num_hits == count);
	ZstLog::app(LogLevel::debug, "Received all messages. Sent: {}, Received:{}", count, test_input->num_hits);

	//Cleanup
	zst_deactivate_entity(test_output);
	zst_deactivate_entity(test_input);
	delete test_output;
	delete test_input;
}


int main(int argc, char **argv)
{
	TestRunner runner("TestBenchmark", argv[0]);

	ZstLog::app(LogLevel::notification, "Starting reliable benchmark test");
	test_benchmark(false);

	ZstLog::app(LogLevel::notification, "Starting unreliable benchmark test");
	test_benchmark(true);

	return 0;
}

