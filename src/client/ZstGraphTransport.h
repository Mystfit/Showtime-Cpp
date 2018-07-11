#pragma once

#include "../core/liasons/ZstPlugLiason.hpp"
#include "../core/ZstTransportLayer.h"
#include "../core/ZstPerformanceMessage.h"

class ZstGraphTransport : 
	public ZstTransportLayer<ZstPerformanceMessage>,
	public ZstPlugLiason
{
public:
	ZstGraphTransport();
	~ZstGraphTransport();
	virtual void init(ZstActor * actor) override;
	virtual void destroy() override;
	void connect_to_client(const char * endpoint_ip);
	void disconnect_from_client();

	const std::string & get_graph_address() const;

	void send_message_impl(ZstMessage * msg) override;
	
private:
	static int s_handle_graph_in(zloop_t *loop, zsock_t *sock, void *arg);
	
	void graph_recv(zmsg_t * msg);

	//Addresses
	std::string first_available_ext_ip();
	std::string m_graph_out_addr;
	std::string m_graph_out_ip;

	//Sockets
	zsock_t * m_graph_in;
	zsock_t * m_graph_out;
};