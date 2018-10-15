#pragma once

#include "../core/liasons/ZstPlugLiason.hpp"
#include "../core/ZstTransportLayer.h"
#include "../core/ZstPerformanceMessage.h"

#define PERFORMANCE_GROUP "p"
#define HANDSHAKE_GROUP "h"

#define SOCK_BUFFER 10000000
#define HWM 2000


class ZstGraphTransport : 
	public ZstTransportLayer<ZstPerformanceMessage>,
	public ZstPlugLiason
{
public:
	ZstGraphTransport();
	~ZstGraphTransport();
	virtual void init() override;
	virtual void destroy() override;
	virtual void connect_to_client(const char * endpoint) = 0;
	virtual void disconnect_from_client() = 0;

	const std::string & get_graph_in_address() const;
	const std::string & get_graph_out_address() const;

	void send_message_impl(ZstMessage * msg) override;

protected:
	ZstActor & actor();

	virtual void init_graph_sockets() = 0;
	virtual void destroy_graph_sockets();
	void attach_graph_sockets(zsock_t * in, zsock_t * out);
	void set_graph_addresses(const std::string & in_addr, const std::string & out_addr);

	std::string first_available_ext_ip() const;

	zsock_t * input_graph_socket();
	zsock_t * output_graph_socket();
	
private:
	static int s_handle_graph_in(zloop_t *loop, zsock_t *sock, void *arg);
	
	void graph_recv(zmsg_t * msg);
	void graph_recv(zframe_t * msg);

	//Actors
	ZstActor m_graph_actor;

	//Addresses
	std::string m_graph_out_addr;
	std::string m_graph_in_addr;

	//Sockets
	//-------

	zsock_t * m_graph_in;
	zsock_t * m_graph_out;
};