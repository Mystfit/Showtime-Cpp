#pragma once

#include "../core/liasons/ZstPlugLiason.hpp"
#include "../core/ZstTransportLayer.h"
#include "../core/ZstPerformanceMessage.h"

#define PERFORMANCE_GROUP "p"
#define HANDSHAKE_GROUP "h"

class ZstGraphTransport : 
	public ZstTransportLayer<ZstPerformanceMessage>,
	public ZstPlugLiason
{
public:
	ZstGraphTransport();
	~ZstGraphTransport();
	virtual void init(ZstActor * actor) override;
	virtual void destroy() override;
	void connect_to_reliable_client(const char * reliable_endpoint);
	void connect_to_unreliable_client(const char * unreliable_endpoint);

	void disconnect_from_client();

	const std::string & get_reliable_graph_address() const;
	const std::string & get_unreliable_graph_address() const;

	void send_message_impl(ZstMessage * msg) override;
	
private:
	static int s_handle_graph_in(zloop_t *loop, zsock_t *sock, void *arg);
	static int s_handle_unreliable_graph_in(zloop_t *loop, zsock_t *sock, void *arg);

	void graph_recv(zmsg_t * msg);
	void graph_recv(zframe_t * msg);

	void init_remote_graph_sockets();
	void init_local_graph_sockets();
	void init_unreliable_graph_sockets();
	void init_graph_sockets(zsock_t * graph_in, zsock_t * graph_out, const std::string & address);

	void destroy_reliable_graph_sockets();
	void destroy_unreliable_graph_sockets();
	void destroy_local_graph_sockets();


	//Addresses
	std::string first_available_ext_ip();	
	std::string m_graph_out_reliable_addr;
	std::string m_graph_in_unreliable_addr;
	std::string m_graph_out_reliable_local_addr;


	//Sockets
	//-------

	//TCP

	zsock_t * m_graph_in_reliable;
	zsock_t * m_graph_out_reliable;

	//UDP

	zsock_t * m_graph_in_unreliable;
	zsock_t * m_graph_out_unreliable;

	//Inproc

	zsock_t * m_graph_in_reliable_local;
	zsock_t * m_graph_out_reliable_local;
};