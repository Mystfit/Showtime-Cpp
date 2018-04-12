#include "ZstCZMQTransportLayer.h"
#include "ZstCZMQMessage.h"
#include "ZstClient.h"

ZstCZMQTransportLayer::ZstCZMQTransportLayer(ZstClient * client) : 
	ZstTransportLayer(client),
	m_graph_out_ip("")
{
}

ZstCZMQTransportLayer::~ZstCZMQTransportLayer()
{
}

void ZstCZMQTransportLayer::destroy()
{
	ZstActor::destroy();
	ZstTransportLayer::destroy();
	zsock_destroy(&m_stage_updates);
	zsock_destroy(&m_stage_router);
	zsock_destroy(&m_graph_in);
	zsock_destroy(&m_graph_out);
	zsys_shutdown();
}

void ZstCZMQTransportLayer::init()
{
	m_startup_uuid = zuuid_new();
	
	//Local dealer socket for receiving messages forwarded from other performers
	m_stage_router = zsock_new(ZMQ_DEALER);
	if (m_stage_router) {
		zsock_set_linger(m_stage_router, 0);
		attach_pipe_listener(m_stage_router, s_handle_stage_router, this);
	}

	//Graph input socket
	m_graph_in = zsock_new(ZMQ_SUB);
	if (m_graph_in) {
		zsock_set_linger(m_graph_in, 0);
		zsock_set_unbounded(m_graph_in);
		attach_pipe_listener(m_graph_in, s_handle_graph_in, this);
	}

	//Stage update socket
	m_stage_updates = zsock_new(ZMQ_SUB);
	if (m_stage_updates) {
		zsock_set_linger(m_stage_updates, 0);
		attach_pipe_listener(m_stage_updates, s_handle_stage_update_in, this);
	}

	std::string network_ip = first_available_ext_ip();
	ZstLog::net(LogLevel::notification, "Using external IP {}", network_ip);

	//Graph output socket
	std::stringstream addr;
	addr << "@tcp://" << network_ip.c_str() << ":*";
	m_graph_out = zsock_new_pub(addr.str().c_str());
	m_graph_out_addr = zsock_last_endpoint(m_graph_out);
	addr.str("");

	//Set graph socket as unbounded by HWM
	zsock_set_unbounded(m_graph_out);
	char * output_ip = zsock_last_endpoint(m_graph_out);
	m_graph_out_ip = std::string(output_ip);
	zstr_free(&output_ip);
	ZstLog::net(LogLevel::notification, "Client graph address: {}", m_graph_out_ip);

	if (m_graph_out)
		zsock_set_linger(m_graph_out, 0);

	//Set up outgoing sockets
	std::string identity = std::string(zuuid_str_canonical(m_startup_uuid));
	ZstLog::net(LogLevel::notification, "Setting socket identity to {}. Length {}", identity, identity.size());

	zsock_set_identity(m_stage_router, identity.c_str());

	//Start the socket/timer reactor
	start_loop();
}

void ZstCZMQTransportLayer::connect_to_stage(std::string stage_address)
{
	m_stage_addr = std::string(stage_address);

	std::stringstream addr;
	addr << "tcp://" << m_stage_addr << ":" << STAGE_ROUTER_PORT;
	m_stage_router_addr = addr.str();

	zsock_connect(m_stage_router, "%s", m_stage_router_addr.c_str());
	addr.str("");

	//Stage subscriber socket for update messages
	addr << "tcp://" << m_stage_addr << ":" << STAGE_PUB_PORT;
	m_stage_updates_addr = addr.str();

	ZstLog::net(LogLevel::notification, "Connecting to stage publisher {}", m_stage_updates_addr);
	zsock_connect(m_stage_updates, "%s", m_stage_updates_addr.c_str());
	zsock_set_subscribe(m_stage_updates, "");
	addr.str("");
}

void ZstCZMQTransportLayer::disconnect_from_stage()
{
	zsock_disconnect(m_stage_updates, "%s", m_stage_updates_addr.c_str());
	zsock_disconnect(m_stage_router, "%s", m_stage_router_addr.c_str());
}

int ZstCZMQTransportLayer::s_handle_graph_in(zloop_t * loop, zsock_t * socket, void * arg) {
	ZstCZMQTransportLayer * transport = (ZstCZMQTransportLayer*)arg;

	//Receive message from graph
	zmsg_t *msg = zmsg_recv(transport->m_graph_in);

	if (client->graph_message_handler(msg) < 0) {
		//TODO: Graph message error. How to handle here if at all?
	}

	zmsg_destroy(&msg);

	return 0;
}

void ZstCZMQTransportLayer::connect_to_client(const char * endpoint_ip, const char * subscription_plug) {
	ZstLog::net(LogLevel::notification, "Connecting to {}. My output endpoint is {}", endpoint_ip, m_graph_out_ip);

	//Connect to endpoint publisher
	zsock_connect(m_graph_in, "%s", endpoint_ip);
	zsock_set_subscribe(m_graph_in, subscription_plug);
}


int ZstCZMQTransportLayer::s_handle_stage_update_in(zloop_t * loop, zsock_t * socket, void * arg) {
	ZstCZMQTransportLayer * transport = (ZstCZMQTransportLayer*)arg;
	ZstMessage * msg = transport->receive_stage_update();
	transport->client()->stage_update_handler(msg);
	return 0;
}


int ZstCZMQTransportLayer::s_handle_stage_router(zloop_t * loop, zsock_t * socket, void * arg) {
	ZstCZMQTransportLayer * transport = (ZstCZMQTransportLayer*)arg;

	//Receive routed message from stage
	ZstMessage * msg = transport->receive_from_stage();

	//Process messages addressed to our client specifically
	if (msg->kind() == ZstMsgKind::GRAPH_SNAPSHOT) {
		ZstLog::net(LogLevel::notification, "Received graph snapshot");
		//Handle graph snapshot synchronisation
		transport->client()->stage_update_handler(msg);
	}
	else if (msg->kind() == ZstMsgKind::SUBSCRIBE_TO_PERFORMER) {
		//Handle connection requests from other clients
		transport->connect_to_client(msg->payload_at(0).data(), msg->payload_at(1).data());
	}
	else if (msg->kind() == ZstMsgKind::OK) {
		//Do nothing?
	}
	else {
		ZstLog::net(LogLevel::notification, "Stage router sent unknown message {}", msg->kind());
	}

	//Process message promises
	transport->client()->msg_dispatch()->process_response_message(msg);

	//Cleanup
	transport->m_pool.release(dynamic_cast<ZstCZMQMessage*>(msg));
	return 0;
}

int ZstCZMQTransportLayer::s_handle_timer(zloop_t * loop, int timer_id, void * arg)
{
	ZstCZMQTransportLayer * transport = (ZstCZMQTransportLayer*)arg;
	transport->m_timers[timer_id]();
	return 0;
}

void ZstCZMQTransportLayer::send_to_performance(ZstMessage * msg)
{
	ZstCZMQMessage * czmq_msg = dynamic_cast<ZstCZMQMessage*>(msg);
	assert(czmq_msg);
	zmsg_t * handle = czmq_msg->handle();
	zmsg_send(&handle, m_graph_out);
}

void ZstCZMQTransportLayer::receive_from_performance()
{
}

ZstMessage * ZstCZMQTransportLayer::get_msg()
{
	ZstMessage * msg = m_pool.get_msg();
	return msg;
}

int ZstCZMQTransportLayer::add_timer(int delay, std::function<void()> timer_func)
{
	m_timers[attach_timer(s_handle_timer, delay, this)] = timer_func;
}

void ZstCZMQTransportLayer::remove_timer(int timer_id)
{
	detach_timer(timer_id);
	m_timers.erase(timer_id);
}

void ZstCZMQTransportLayer::send_to_stage(ZstMessage * msg)
{
	ZstCZMQMessage * czmq_msg = dynamic_cast<ZstCZMQMessage*>(msg);

	//Message needs to be a ZstCZMQMessage else something has gone wrong
	assert(czmq_msg);

	//Dealer socket doesn't add an empty frame to seperate identity chain and payload, so we handle it here
	zframe_t * empty = zframe_new_empty();
	zmsg_t * msg_handle = czmq_msg->handle();
	zmsg_prepend(msg_handle, &empty);
	zmsg_send(&msg_handle, m_stage_router);
	m_pool.release(czmq_msg);
}

ZstMessage * ZstCZMQTransportLayer::receive_stage_update()
{
	ZstCZMQMessage * msg = NULL;
	zmsg_t * recv_msg = zmsg_recv(m_stage_updates);
	if (recv_msg) {
		msg = static_cast<ZstCZMQMessage*>(m_pool.get_msg());
		msg->unpack(recv_msg);
	}
	return msg;
}

ZstMessage * ZstCZMQTransportLayer::receive_from_stage() {
	ZstCZMQMessage * msg = NULL;

	zmsg_t * recv_msg = zmsg_recv(m_stage_router);
	if (recv_msg) {
		msg = static_cast<ZstCZMQMessage*>(m_pool.get_msg());

		//Pop blank seperator frame left from the dealer socket
		zframe_t * empty = zmsg_pop(recv_msg);
		zframe_destroy(&empty);

		//Unpack message contents
		msg->unpack(recv_msg);
	}
	return msg;
}

std::string ZstCZMQTransportLayer::first_available_ext_ip() {
	ziflist_t * interfaces = ziflist_new();
	std::string interface_ip_str = "127.0.0.1";

	if (ziflist_first(interfaces) != NULL) {
		interface_ip_str = std::string(ziflist_address(interfaces));
	}

	return interface_ip_str;
}