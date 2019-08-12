#pragma once

#include "../core/ZstActor.h"
#include "../core/ZstStageMessage.h"
#include "../core/transports/ZstTransportLayer.h"

class ZstZMQServerTransport :
	public ZstTransportLayer<ZstStageMessage>
{
public:
	ZstZMQServerTransport();
	~ZstZMQServerTransport();
	void init(int port);
	void destroy() override;

	//Incoming socket handlers
	static int s_handle_router(zloop_t *loop, zsock_t *sock, void *arg);

	void send_message_impl(ZstMessage * msg) override;
	void on_receive_msg(ZstMessage * msg) override;

private:
	void init() override;
	ZstActor m_server_actor;
	zsock_t * m_clients_sock;
};