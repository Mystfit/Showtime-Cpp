#pragma once

#include <string>
#include <vector>
#include <memory>
#include "ZstExports.h"
#include "czmq.h"
#include "ZstInstrument.h"

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
		ZST_EXPORT vector<shared_ptr<ZstInstrument>>& get_instruments();

	private:
		//Name property
		string m_name;

		//All instruments owned by this section
		vector<shared_ptr<ZstInstrument>> m_instruments;


		//Zeromq members
		zloop_t *m_loop;	
		zsock_t *m_toStage;		//Reqests sent to the stage
		zsock_t *m_fromStage;	//Requests from the stage
		zsock_t *m_graph_out;	//Pub for sending graph updates
		zsock_t *m_graph_in;	//Sub for receiving graph updates
	};
}

