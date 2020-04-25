#pragma once

#include "../core/adaptors/ZstStageTransportAdaptor.hpp"
#include "../core/ZstActor.h"
#include <boost/uuid/uuid.hpp>
#include "../core/transports/ZstStageTransport.h"

using namespace boost::uuids;

namespace showtime {

class ZstZMQClientTransport : public ZstStageTransport
{
public:
	ZstZMQClientTransport();
	~ZstZMQClientTransport();
	virtual void init() override;
	virtual void destroy() override;
	virtual void connect(const std::string & stage_address) override;
	virtual void disconnect() override;
    
private:
	void send_message_impl(const uint8_t * msg_buffer, size_t msg_buffer_size, const ZstTransportArgs & args) const override;
	static int s_handle_stage_router(zloop_t *loop, zsock_t *sock, void *arg);
	void sock_recv(zsock_t* socket);

	//Sockets
	zsock_t * m_server_sock;

	//Addresses
	std::string m_server_addr;

	//Actor
	ZstActor m_client_actor;

	//Id
	boost::uuids::uuid m_origin_endpoint_UUID;
};

}
