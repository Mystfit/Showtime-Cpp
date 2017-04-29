#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include "ZstExports.h"
#include "czmq.h"
#include "ZstInstrument.h"
#include "ZstMessages.hpp"

using namespace std;

namespace Showtime {

	class ZstSection
	{
	public:
		ZstSection(string name);
		ZST_EXPORT ~ZstSection();

		//Factory
		ZST_EXPORT static unique_ptr<ZstSection> create_section(string name);

		// Creates a new instrument
		ZST_EXPORT shared_ptr<ZstInstrument> create_instrument(string name);

		// Removes and destroys an instrument
		ZST_EXPORT void destroy_instrument(ZstInstrument& instrument);

		//List of all instruments owned by this section
		ZST_EXPORT vector<shared_ptr<ZstInstrument> >& get_instruments();
        
        ZST_EXPORT void register_to_stage();

	private:
		//Name property
		string m_name;

		//All instruments owned by this section
		vector<shared_ptr<ZstInstrument> > m_instruments;

		//Zeromq members
		zloop_t *m_loop;	
		zsock_t *m_stage;		//Reqests sent to the stage server
        zsock_t *m_reply;		//Reqests from the stage server

		zsock_t *m_graph_out;	//Pub for sending graph updates
		zsock_t *m_graph_in;	//Sub for receiving graph updates
        
        //Heartbeat timer
        void start_heartbeat();
        static int s_heartbeat_timer(zloop_t *loop, int timer_id, void *arg);
        
        void start_client();
        void send_heartbeat();
	};
}

