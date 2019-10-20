#pragma once

#include "../core/adaptors/ZstStageTransportAdaptor.hpp"
#include "../core/ZstStageMessage.h"
#include "../core/transports/ZstStageTransport.h"
#include "../core/ZstActor.h"

#include <czmq.h>
#include <boost/uuid/uuid.hpp>

using namespace boost::uuids;

namespace showtime {

class ZstZMQClientTransport : 
	public ZstStageTransport
{
public:
	ZstZMQClientTransport();
	~ZstZMQClientTransport();
	virtual void init() override;
	virtual void destroy() override;
	virtual void connect(const std::string & stage_address) override;
	virtual void disconnect() override;
    
    virtual ZstMessageReceipt send_msg(Content message_type, flatbuffers::Offset<void> message_content, flatbuffers::FlatBufferBuilder & buffer_builder, const ZstTransportArgs& args) override;

private:
	void send_message_impl(const uint8_t * msg_buffer, size_t msg_buffer_size, const ZstTransportArgs & args) const override;
	void sock_recv(zsock_t* socket, bool pop_first);
	static int s_handle_stage_router(zloop_t *loop, zsock_t *sock, void *arg);
	
	//Sockets
	zsock_t * m_server_sock;

	//Addresses
	std::string m_server_addr;

	//Actor
	ZstActor m_client_actor;

	//Id
	boost::uuids::uuid m_endpoint_UUID;
};

}
