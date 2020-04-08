#pragma once

#include <mutex>
#include "../core/ZstActor.h"
#include "../core/ZstStageMessage.h"
#include "../core/transports/ZstStageTransport.h"

namespace showtime {

class ZstZMQServerTransport : public ZstStageTransport
{
public:
	ZstZMQServerTransport();
	~ZstZMQServerTransport();
	void init() override;
	void destroy() override;
	virtual int bind(const std::string& address) override;
	int port();

	//Incoming socket handlers
	static int s_handle_router(zloop_t *loop, zsock_t *sock, void *arg);
	void sock_recv(zsock_t* socket);

	void send_message_impl(const uint8_t* msg_buffer, size_t msg_buffer_size, const ZstTransportArgs& args) const override;

private:
	int m_port;
	mutable std::mutex m_transport_mtx;
	ZstActor m_server_actor;
	zsock_t * m_clients_sock;
};

}