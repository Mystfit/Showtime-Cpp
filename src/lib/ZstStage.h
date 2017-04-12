#pragma once
#include "ZstExports.h"
#include "czmq.h"
#include <string>
#include <vector>
#include <iostream>

using namespace std;

namespace Showtime {
	class ZstStage {
	public:
		ZstStage();
		ZST_EXPORT ~ZstStage();

		ZST_EXPORT static ZstStage* create_stage();

		ZST_EXPORT void start();

		int dealer_port = 6000;
		int router_port = 6001;

	private:

		static void thread_loop_func(zsock_t *pipe, void *args);
		zactor_t *m_loop_actor;

		zsock_t *m_section_router;
		vector<zsock_t*> m_client_command_pipes;

		zsock_t *m_graph_update_pub;
		zloop_t *m_reactor;

		static int s_handle_router(zloop_t *loop, zmq_pollitem_t *poller, void *arg);
	};
}