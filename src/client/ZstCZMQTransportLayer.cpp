#include "ZstCZMQTransportLayer.h"

ZstCZMQTransportLayer::ZstCZMQTransportLayer() : 
	m_graph_out_ip("")
{
}

int ZstCZMQTransportLayer::s_handle_graph_in(zloop_t * loop, zsock_t * socket, void * arg) {
	ZstClient *client = (ZstClient*)arg;

	//Receive message from graph
	zmsg_t *msg = zmsg_recv(client->m_graph_in);

	if (client->graph_message_handler(msg) < 0) {
		//TODO: Graph message error. How to handle here if at all?
	}

	zmsg_destroy(&msg);

	return 0;
}


int ZstCZMQTransportLayer::s_handle_stage_update_in(zloop_t * loop, zsock_t * socket, void * arg) {
	ZstClient *client = (ZstClient*)arg;
	ZstMessage * msg = client->receive_stage_update();
	client->stage_update_handler(msg);
	return 0;
}


int ZstCZMQTransportLayer::s_handle_stage_router(zloop_t * loop, zsock_t * socket, void * arg) {
	ZstClient *client = (ZstClient*)arg;

	//Receive routed message from stage
	ZstMessage * msg = client->receive_from_stage();

	//Process messages addressed to our client specifically
	if (msg->kind() == ZstMsgKind::GRAPH_SNAPSHOT) {
		ZstLog::net(LogLevel::notification, "Received graph snapshot");
		//Handle graph snapshot synchronisation
		client->stage_update_handler(msg);
	}
	else if (msg->kind() == ZstMsgKind::SUBSCRIBE_TO_PERFORMER) {
		//Handle connection requests from other clients
		client->connect_client_handler(msg->payload_at(0).data(), msg->payload_at(1).data());
	}
	else if (msg->kind() == ZstMsgKind::OK) {
		//Do nothing?
	}
	else {
		ZstLog::net(LogLevel::notification, "Stage router sent unknown message {}", msg->kind());
	}

	//Process message promises
	client->msg_pool().process_response_message(msg);

	//Cleanup
	client->msg_pool().release(msg);
	return 0;
}



void ZstCZMQTransportLayer::send_to_performance(ZstPlug * plug)
{
	zmsg_t *msg = zmsg_new();

	//Add output plug path
	zmsg_addstr(msg, plug->URI().path());

	//Pack value into stream
	std::stringstream s;
	plug->raw_value()->write(s);
	zframe_t *payload = zframe_new(s.str().c_str(), s.str().size());
	zmsg_append(msg, &payload);

	//Send it
	zmsg_send(&msg, m_graph_out);
}

void ZstCZMQTransportLayer::send_to_stage(ZstMessage * msg)
{
	//Dealer socket doesn't add an empty frame to seperate identity chain and payload, so we handle it here
	zframe_t * empty = zframe_new_empty();
	zmsg_t * msg_handle = msg->handle();
	zmsg_prepend(msg_handle, &empty);
	msg->send(m_stage_router);
	msg_pool().release(msg);
}

ZstMessage * ZstCZMQTransportLayer::receive_stage_update()
{
	ZstMessage * msg = NULL;
	zmsg_t * recv_msg = zmsg_recv(m_stage_updates);
	if (recv_msg) {
		msg = msg_pool().get();
		msg->unpack(recv_msg);
	}
	return msg;
}

ZstMessage * ZstCZMQTransportLayer::receive_from_stage() {
	ZstMessage * msg = NULL;

	zmsg_t * recv_msg = zmsg_recv(m_stage_router);
	if (recv_msg) {
		msg = msg_pool().get();

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