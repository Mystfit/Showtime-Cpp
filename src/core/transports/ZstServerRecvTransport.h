#pragma once

#include "../ZstActor.h"
#include "../ZstStageMessage.h"
#include "ZstTransportLayer.h"
#include "entities/ZstPerformer.h"

class ZstServerRecvTransport :
	public ZstTransportLayer<ZstStageMessage>
{
public:
	ZstServerRecvTransport();
	~ZstServerRecvTransport();
	void init(int port);
	void destroy() override;

	//Incoming socket handlers
	static int s_handle_router(zloop_t *loop, zsock_t *sock, void *arg);

	void send_message_impl(ZstMessage * msg) override;
	void on_receive_msg(ZstMessage * msg) override;

private:
	void init() override;
	ZstActor m_router_actor;
	zsock_t * m_performer_router;
};