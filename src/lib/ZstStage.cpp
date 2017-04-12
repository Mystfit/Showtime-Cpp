#include "ZstStage.h"

using namespace Showtime;

ZstStage::ZstStage()
{
	m_section_router = zsock_new_router("@tcp://127.0.0.1:6000");
	m_graph_update_pub = zsock_new_pub("@tcp://127.0.0.1:*");
}

ZstStage::~ZstStage()
{
}


ZstStage* ZstStage::create_stage()
{
	return new ZstStage();
}

void ZstStage::start()
{
	m_reactor = zloop_new();
	zloop_set_verbose(m_reactor, true);
	zloop_set_nonstop(m_reactor, true);

	zmq_pollitem_t poller = { m_section_router, 0, ZMQ_POLLIN };
	zloop_poller(m_reactor, &poller, s_handle_router, NULL);	//BREAKING?!

	zactor_t *m_loop_actor = zactor_new(thread_loop_func, m_reactor);
}

void ZstStage::thread_loop_func(zsock_t *pipe, void *args)
{
	zloop_start((zloop_t *)args);
}

int ZstStage::s_handle_router(zloop_t * loop, zmq_pollitem_t * poller, void * arg)
{
	char *msg = zstr_recv(poller->socket);
	cout << msg << endl;

	return 0;
}
