#pragma once

#include "../core/ZstTransportLayer.h"
#include "../core/ZstActor.h"
#include "../core/ZstStageMessage.h"

#define HEARTBEAT_DURATION 1000
#define MAX_MISSED_HEARTBEATS 10
#define STAGE_MESSAGE_POOL_BLOCK 512

class ZstStagePublisherTransport :
	public ZstTransportLayer<ZstStageMessage>,
	public ZstActor
{
public:
	ZstStagePublisherTransport();
	~ZstStagePublisherTransport();
	void init(ZstActor * actor) override;
	void destroy() override;

	void send_message_impl(ZstMessage * msg) override;

	void on_receive_msg(ZstMessage * msg);

private:
	zsock_t * m_graph_update_pub;
};