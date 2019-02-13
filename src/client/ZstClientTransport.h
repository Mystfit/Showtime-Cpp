#pragma once

#include "../core/ZstEventDispatcher.hpp"
#include "../core/adaptors/ZstTransportAdaptor.hpp"
#include "../core/transports/ZstTransportLayer.h"
#include "../core/ZstStageMessage.h"
#include "../core/ZstMessageSupervisor.hpp"

class ZstClientTransport : 
	public ZstTransportLayer<ZstStageMessage>,
	public ZstMessageSupervisor
{
public:
	ZstClientTransport();
	~ZstClientTransport();
	virtual void init() override;
	virtual void destroy() override;
	void connect_to_stage(const std::string stage_address);
	void disconnect_from_stage();
	void process_events() override;
	void begin_send_message(ZstMessage * msg, const ZstTransportSendType & sendtype, const MessageReceivedAction & action) override;

private:
	void send_message_impl(ZstMessage * msg) override;
	void sock_recv(zsock_t* socket, bool pop_first);
	static int s_handle_stage_router(zloop_t *loop, zsock_t *sock, void *arg);

	void on_receive_msg(ZstMessage * msg) override;

	virtual ZstMessageReceipt send_sync_message(ZstStageMessage * msg);
	virtual void send_async_message(ZstStageMessage * msg, const MessageReceivedAction & completed_action);

	//Sockets
	zsock_t * m_stage_router;

	//Addresses
	std::string m_stage_addr;
	std::string m_stage_router_addr;

	ZstActor m_client_actor;
};
