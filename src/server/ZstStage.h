#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <boost/thread/thread.hpp>

//Showtime API
#include <ZstCore.h>

//Core headers
#include "../core/ZstActor.h"
#include "../core/ZstStageMessage.h"
#include "../core/adaptors/ZstTransportAdaptor.hpp"

//Stage headers
#include "ZstStageSession.h"
#include "ZstStagePublisherTransport.h"
#include "ZstStageRouterTransport.h"


class ZstStage : 
	public ZstTransportAdaptor,
	public ZstEventDispatcher<ZstTransportAdaptor*>
{
public:
	ZstStage();
	~ZstStage();
	void init_stage(const char * stage_name, bool threaded);
	void destroy();
	bool is_destroyed();	
	void process_events();
	
private:
	bool m_is_destroyed;
	boost::thread m_eventloop_thread;

	int m_heartbeat_timer_id;
	void stage_heartbeat_timer_func();

	ZstActor m_timer_actor;
	ZstStageSession * m_session;
	
	ZstStagePublisherTransport * m_publisher_transport;
	ZstStageRouterTransport * m_router_transport;
};


struct ZstStageLoop {
public:
	ZstStageLoop(ZstStage * stage);
	void operator()();

private:
	ZstStage * m_stage;
};