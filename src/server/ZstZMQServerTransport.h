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
	void init() override;
	void destroy() override;
	virtual void bind(const std::string& address) override;

	//Incoming socket handlers
	static int s_handle_router(zloop_t *loop, zsock_t *sock, void *arg);

	void send_message_impl(ZstMessage * msg) override;
	void receive_msg(ZstMessage * msg) override;

private:
	ZstActor m_server_actor;
	zsock_t * m_clients_sock;
};