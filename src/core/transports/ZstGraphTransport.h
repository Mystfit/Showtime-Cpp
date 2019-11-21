#pragma once

#include "../ZstPerformanceMessage.h"
#include "../adaptors/ZstGraphTransportAdaptor.hpp"
#include "../ZstMessagePool.hpp"
#include "../ZstActor.h"
#include "ZstTransportLayerBase.hpp"
#include "ZstExports.h"

#define PERFORMANCE_GROUP "p"
#define HANDSHAKE_GROUP "h"

#define SOCK_BUFFER 10000000
#define HWM 2000

namespace showtime {

class ZstGraphTransport :
    public ZstTransportLayerBase<ZstPerformanceMessage, ZstGraphTransportAdaptor>,
    public ZstGraphTransportAdaptor
{
public:
	ZST_EXPORT ZstGraphTransport();
	ZST_EXPORT ~ZstGraphTransport();
	ZST_EXPORT virtual void init() override;
	ZST_EXPORT virtual void destroy() override;

	ZST_EXPORT const std::string & get_graph_in_address() const;
	ZST_EXPORT const std::string & get_graph_out_address() const;

    ZST_EXPORT virtual ZstMessageReceipt send_msg(flatbuffers::Offset<GraphMessage> message_content, flatbuffers::FlatBufferBuilder& buffer_builder, const ZstTransportArgs& args) override;
	ZST_EXPORT void send_message_impl(const uint8_t * msg_buffer, size_t msg_buffer_size, const ZstTransportArgs & args) const override;

protected:
	ZST_EXPORT ZstActor & actor();

	ZST_EXPORT virtual void init_graph_sockets() = 0;
	ZST_EXPORT virtual void destroy_graph_sockets();
	ZST_EXPORT void attach_graph_sockets(zsock_t * in, zsock_t * out);
	ZST_EXPORT void set_graph_addresses(const std::string & in_addr, const std::string & out_addr);

	ZST_EXPORT std::string first_available_ext_ip() const;

	ZST_EXPORT zsock_t * input_graph_socket() const;
	ZST_EXPORT zsock_t * output_graph_socket() const;
	ZST_EXPORT virtual void dispatch_receive_event(std::shared_ptr<ZstPerformanceMessage> msg, ZstEventCallback on_complete) override;
	
private:
	void sock_recv(zsock_t* socket);
	static int s_handle_graph_in(zloop_t *loop, zsock_t *sock, void *arg);
	
	//Actors
	ZstActor m_graph_actor;

	//Addresses
	std::string m_graph_out_addr;
	std::string m_graph_in_addr;

	//Sockets
	zsock_t * m_graph_in;
	zsock_t * m_graph_out;
};

}
