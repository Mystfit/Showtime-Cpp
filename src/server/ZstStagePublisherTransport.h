#pragma once

#include "../core/ZstTransportLayer.h"
#include "../core/ZstActor.h"
#include "../core/ZstStageMessage.h"


class ZstStagePublisherTransport :
	public ZstTransportLayer<ZstStageMessage>,
	public ZstActor
{
public:
	ZstStagePublisherTransport();
	~ZstStagePublisherTransport();
	void init() override;
	void destroy() override;

	void send_message_impl(ZstMessage * msg) override;

	void on_receive_msg(ZstMessage * msg) override;

private:
	zsock_t * m_graph_update_pub;
};