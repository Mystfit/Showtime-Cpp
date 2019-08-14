#pragma once

#include "../core/adaptors/ZstTransportAdaptor.hpp"
#include "../core/ZstStageMessage.h"
#include "../core/transports/ZstTransportLayer.h"

class ZstZMQClientTransport : 
	public ZstTransportLayer<ZstStageMessage>
{
public:
	ZstZMQClientTransport();
	~ZstZMQClientTransport();
	virtual void init() override;
	virtual void destroy() override;
	virtual void connect(const std::string & stage_address) override;
	virtual void disconnect() override;
	void process_events() override;

private:
	void send_message_impl(ZstMessage * msg) override;
	void sock_recv(zsock_t* socket, bool pop_first);
	static int s_handle_stage_router(zloop_t *loop, zsock_t *sock, void *arg);
	void receive_msg(ZstMessage * msg) override;
	
	//Sockets
	zsock_t * m_server_sock;

	//Addresses
	std::string m_stage_addr;
	std::string m_server_addr;

	ZstActor m_client_actor;
};
