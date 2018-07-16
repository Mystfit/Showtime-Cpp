#include <string>
#include <chrono>
#include <vector>
#include <memory>
#include <tuple>
#include <iostream>
#include <sstream>
#include <exception>

#ifdef WIN32
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>  
#include <crtdbg.h>  
#pragma warning(push) 
#pragma warning(disable:4189 4996)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic pop
#endif
#include <boost/process.hpp>
#include <boost/thread/thread.hpp>
#include <boost/filesystem.hpp>
using namespace boost::process;

#ifdef WIN32
#pragma warning(pop)
#endif

#include <Showtime.h>

#define BOOST_THREAD_DONT_USE_DATETIME
#define MEM_LEAK_LOOPS 200000;


class TestAdaptor {
public:
	TestAdaptor() : m_num_calls(0) {};

	int num_calls() {
		return m_num_calls;
	}

	void reset_num_calls() {
		m_num_calls = 0;
	}

	void inc_calls() {
		m_num_calls++;
	}

private:
	int m_num_calls;
};


class OutputComponent : public ZstComponent {
private:
	ZstOutputPlug * m_output;

public:
	OutputComponent(const char * name) : ZstComponent("TESTER", name) {
		m_output = create_output_plug("out", ZstValueType::ZST_FLOAT);
	}

	void on_activation() override {
		ZstLog::app(LogLevel::debug, "{} on_activation()", URI().path());
	}

	void on_deactivation() override {
		ZstLog::app(LogLevel::debug, "{} on_deactivation()", URI().path());
	}

	virtual void compute(ZstInputPlug * plug) override {}

	void send(float val) {
		m_output->append_float(val);
		m_output->fire();
	}

	ZstOutputPlug * output() {
		return m_output;
	}
};


class InputComponent : public ZstComponent {
private:
	ZstInputPlug * m_input;

public:
	int num_hits = 0;
	int compare_val = 0;
	int last_received_val = 0;
	bool log = false;

	InputComponent(const char * name, int cmp_val, bool should_log = false) :
		ZstComponent("TESTER", name), compare_val(cmp_val)
	{
		log = should_log;
		m_input = create_input_plug("in", ZstValueType::ZST_FLOAT);
	}

	void on_activation() override {
		ZstLog::app(LogLevel::debug, "{} on_activation()", URI().path());
	}

	void on_deactivation() override {
		ZstLog::app(LogLevel::debug, "{} on_deactivation()", URI().path());
	}

	virtual void compute(ZstInputPlug * plug) override {
		float actual_val = plug->float_at(0);
		last_received_val = int(actual_val);
		if (log) {
			ZstLog::app(LogLevel::debug, "Input filter received value {0:d}", last_received_val);
		}
		num_hits++;
	}

	ZstInputPlug * input() {
		return m_input;
	}

	void reset() {
		num_hits = 0;
	}
};


int main(int argc, char **argv) {

	//Create server
	std::string ext_test_folder = boost::filesystem::system_complete(argv[0]).parent_path().generic_string();
	boost::process::pipe server_in;
	child server_process;

	std::string prog = ext_test_folder + "/ShowtimeServer";
#ifdef WIN32
	prog += ".exe";
#endif

	std::string test_flag = "-t";
	try {
		server_process = boost::process::child(prog, test_flag, std_in < server_in);
	}
	catch (process_error e) {
		ZstLog::app(LogLevel::debug, "Server process failed to start. Code:{} Message:{}", e.code().value(), e.what());
	}
	assert(server_process.valid());

	//Start memleak test
	zst_init("memleak_test", true);
	zst_join("127.0.0.1");
	ZstLog::app(LogLevel::notification, "Starting memory leak test");

	ZstLog::app(LogLevel::debug, "Creating entities and cables");
	OutputComponent * test_output = new OutputComponent("memleak_test_out");
	InputComponent * test_input = new InputComponent("memleak_test_in", 10, false);
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
	zst_destroy();
	

	std::string term_msg = "$TERM\n";
	server_in.write(term_msg.c_str(), term_msg.size());
	server_process.wait();

	std::cout << "Memory leak test completed" << std::endl;
	return 0;
}
