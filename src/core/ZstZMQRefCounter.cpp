#include "ZstZMQRefCounter.h"
#include <czmq.h>

static int s_total_ref_count = 0;
static std::unordered_map<std::string, int> s_zmq_ref_counts;

void zst_zmq_inc_ref_count(const char* ref_name)
{
	s_total_ref_count++;
	s_zmq_ref_counts[ref_name]++;
	
	//Log::net(Log::Level::debug, "Total ZeroMQ references: {}", s_ref_count);
}

void zst_zmq_dec_ref_count(const char* ref_name)
{
	s_total_ref_count--;
	s_zmq_ref_counts[ref_name]--;

	//Log::net(Log::Level::debug, "ZeroMQ references remaining: {}", s_ref_count);
	if (s_total_ref_count <= 0) {
		//Log::net(Log::Level::debug, "No more ZeroMQ references. Shutting down ZeroMQ.");
		zsys_shutdown();
	}
}

int zst_zmq_total_ref_count() {
	return s_total_ref_count;
}


std::unordered_map<std::string, int> zst_zmq_ref_counts() {
	return s_zmq_ref_counts;
}