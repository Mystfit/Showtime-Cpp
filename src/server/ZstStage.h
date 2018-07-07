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
#include "../core/ZstTransportDispatcher.h"
#include "../core/ZstStageMessage.h"

//Stage headers
#include "ZstStageSession.h"
#include "ZstStageTransport.h"


class ZstStage : 
	public ZstActor,
	public ZstStageDispatchAdaptor
{
public:
	ZstStage();
	~ZstStage();
	void init(const char * stage_name);
	void destroy() override;
	bool is_destroyed();	

	void process_events();
	void flush_events();

	void on_receive_msg(ZstStageMessage * msg) override;

private:
	bool m_is_destroyed;

	int m_heartbeat_timer_id;
	static int stage_heartbeat_timer_func(zloop_t * loop, int timer_id, void * arg);

   	//Outgoing events
	ZstStageMessage * synchronise_client_graph(ZstPerformer * client);

	ZstStageSession * m_session;
	ZstStageTransport * m_transport;
	ZstTransportDispatcher * m_dispatch;

	ZstEventDispatcher<ZstStageDispatchAdaptor*> m_stage_events;
};
