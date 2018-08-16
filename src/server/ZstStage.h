#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <boost/thread/thread.hpp>
#include <boost/asio.hpp>

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


struct ZstStageIOLoop {
public:
	ZstStageIOLoop(ZstStage * stage);
	void operator()();
	boost::asio::io_service & IO_context();

private:
	boost::asio::io_service m_io;
	ZstStage * m_stage;
};


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
	boost::thread m_stage_eventloop_thread;
	ZstStageIOLoop m_stage_eventloop;

	static void stage_heartbeat_timer(boost::asio::deadline_timer * t, ZstStage * stage, boost::posix_time::milliseconds duration);
	boost::asio::deadline_timer m_heartbeat_timer;

	ZstStageSession * m_session;
	
	ZstStagePublisherTransport * m_publisher_transport;
	ZstStageRouterTransport * m_router_transport;
};
