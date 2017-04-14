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

		int dealer_port = 6000;
		int router_port = 6001;

	private:

		static void thread_loop_func(zsock_t *pipe, void *args);
        void event_loop();
        
        //Pipes
		zsock_t *m_section_router;
		zsock_t *m_graph_update_pub;
        vector<zsock_t*> m_section_pipes;
        
        //Let's get it started in HAH
        void start();

        
        //Polling
        zpoller_t *m_poller;
        zloop_t *m_loop;
        zactor_t *m_loop_actor;

		static int s_handle_router(zloop_t *loop, zsock_t *sock, void *arg);
	};
}
