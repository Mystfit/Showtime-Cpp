#include "ZstZMQRefCounter.h"
#include <czmq.h>

static int s_ref_count = 0;

void zst_zmq_inc_ref_count()
{
	s_ref_count++;
}

void zst_zmq_dec_ref_count()
{
	s_ref_count--;
	if (s_ref_count <= 0) {
		zsys_shutdown();
	}
}
