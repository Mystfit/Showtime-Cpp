#pragma once

#include <ZstEventDispatcher.hpp>
#include "../core/adaptors/ZstTransportAdaptor.hpp"
#include "../core/ZstTransportLayer.h"
#include "../core/ZstStageMessage.h"

class ZstClientTransport : 
	public ZstTransportLayer<ZstStageMessage>
{
public:
	ZstClientTransport();
	~ZstClientTransport();
	virtual void init() override;
	virtual void destroy() override;
	void connect_to_stage(std::string stage_address);
	void disconnect_from_stage();

private:
	void send_message_impl(ZstMessage * msg) override;

	static int s_handle_stage_update_in(zloop_t *loop, zsock_t *sock, void *arg);
	static int s_handle_stage_router(zloop_t *loop, zsock_t *sock, void *arg);

	void on_receive_msg(ZstMessage * msg) override;

	//Sockets
	zsock_t * m_stage_router;
	zsock_t * m_stage_updates;

	//Addresses
	std::string m_stage_addr;
	std::string m_stage_router_addr;
	std::string m_stage_updates_addr;

	zuuid_t * m_startup_uuid;
	ZstActor m_client_actor;
};
