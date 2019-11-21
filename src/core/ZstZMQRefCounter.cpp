#include "ZstZMQRefCounter.h"
#include "ZstLogging.h"
#include <czmq.h>

static int s_ref_count = 0;

void zst_zmq_inc_ref_count()
{
	s_ref_count++;
	//ZstLog::net(LogLevel::debug, "Total ZeroMQ references: {}", s_ref_count);
}

void zst_zmq_dec_ref_count()
{
	s_ref_count--;
	//ZstLog::net(LogLevel::debug, "ZeroMQ references remaining: {}", s_ref_count);
	if (s_ref_count <= 0) {
		ZstLog::net(LogLevel::debug, "No more ZeroMQ references. Shutting down ZeroMQ.");
		zsys_shutdown();
	}
}

int zst_zmq_ref_count() {
	return s_ref_count;
}
