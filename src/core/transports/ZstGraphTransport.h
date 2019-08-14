#pragma once

#include "../ZstPerformanceMessage.h"
#include "ZstTransportLayer.h"
#include "ZstExports.h"

#define PERFORMANCE_GROUP "p"
#define HANDSHAKE_GROUP "h"

#define SOCK_BUFFER 10000000
#define HWM 2000


class ZstGraphTransport : 
	public ZstTransportLayer<ZstPerformanceMessage>
{
public:
	ZST_EXPORT ZstGraphTransport();
	ZST_EXPORT ~ZstGraphTransport();
	ZST_EXPORT virtual void init() override;
	ZST_EXPORT virtual void destroy() override;

	ZST_EXPORT virtual void connect(const std::string & address) = 0;
	ZST_EXPORT virtual void bind(const std::string& address) = 0;
	ZST_EXPORT virtual void disconnect() = 0;

	ZST_EXPORT const std::string & get_graph_in_address() const;
	ZST_EXPORT const std::string & get_graph_out_address() const;

	ZST_EXPORT void send_message_impl(ZstMessage * msg) override;

protected:
	ZST_EXPORT ZstActor & actor();

	ZST_EXPORT virtual void init_graph_sockets() = 0;
	ZST_EXPORT virtual void destroy_graph_sockets();
	ZST_EXPORT void attach_graph_sockets(zsock_t * in, zsock_t * out);
	ZST_EXPORT void set_graph_addresses(const std::string & in_addr, const std::string & out_addr);

	ZST_EXPORT std::string first_available_ext_ip() const;

	ZST_EXPORT zsock_t * input_graph_socket();
	ZST_EXPORT zsock_t * output_graph_socket();
	
private:
	static int s_handle_graph_in(zloop_t *loop, zsock_t *sock, void *arg);
	
	void graph_recv(zframe_t * msg);

	//Actors
	ZstActor m_graph_actor;

	//Addresses
	std::string m_graph_out_addr;
	std::string m_graph_in_addr;

	//Sockets
	zsock_t * m_graph_in;
	zsock_t * m_graph_out;
};