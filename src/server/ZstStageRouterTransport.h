#pragma once

#include <entities/ZstPerformer.h>
#include "../core/transports/ZstTransportLayer.h"
#include "../core/ZstActor.h"
#include "../core/ZstStageMessage.h"


class ZstStageRouterTransport :
	public ZstTransportLayer<ZstStageMessage>
{
public:
	ZstStageRouterTransport();
	~ZstStageRouterTransport();
	void init(std::shared_ptr<ZstActor> reactor) override;
	void destroy() override;

	//Incoming socket handlers
	static int s_handle_router(zloop_t *loop, zsock_t *sock, void *arg);

	void send_message_impl(ZstMessage * msg) override;

	void on_receive_msg(ZstMessage * msg) override;

private:
	zsock_t * m_performer_router;
};