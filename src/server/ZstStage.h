#pragma once

#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

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

//Forwards
class ZstBoostEventWakeup;



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
	boost::thread m_stage_timer_thread;
	boost::asio::io_context m_io;
	void timer_loop();
	void event_loop();
	std::shared_ptr<ZstBoostEventWakeup> m_event_condition;

	//Timers
	static void stage_heartbeat_timer(boost::asio::deadline_timer * t, ZstStage * stage, boost::posix_time::milliseconds duration);
	boost::asio::deadline_timer m_heartbeat_timer;

	//Modules
	ZstStageSession * m_session;
	
	//Transports
	ZstStagePublisherTransport * m_publisher_transport;
	ZstStageRouterTransport * m_router_transport;
	std::shared_ptr<ZstActor> m_reactor;
};
