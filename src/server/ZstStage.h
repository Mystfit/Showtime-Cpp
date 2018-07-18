#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cf/cfuture.h>
#include <cf/time_watcher.h>
#include "czmq.h"

//Showtime API
#include <ZstCore.h>

//Core headers
#include "../core/ZstActor.h"
#include "../core/ZstStageMessage.h"

//Stage headers
#include "ZstStageSession.h"
#include "ZstStageTransport.h"


class ZstStage : 
	public ZstStageDispatchAdaptor
{
public:
	ZstStage();
	~ZstStage();
	void init_stage(const char * stage_name);
	void destroy() override;
	bool is_destroyed();	

	void process_events();
	void flush_events();

	void on_receive_msg(ZstStageMessage * msg) override;

private:
	bool m_is_destroyed;

	int m_heartbeat_timer_id;
	void stage_heartbeat_timer_func();

   	//Outgoing events
	ZstStageMessage * synchronise_client_graph(ZstPerformer * client);

	ZstActor * m_actor;
	ZstStageSession * m_session;
	ZstStageTransport * m_transport;

	ZstEventDispatcher<ZstStageDispatchAdaptor*> m_stage_events;
};
