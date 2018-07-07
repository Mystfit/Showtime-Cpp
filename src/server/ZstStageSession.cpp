#include "ZstStageSession.h"


ZstStageSession::ZstStageSession() : m_hierarchy(NULL)
{
	m_hierarchy = new ZstStageHierarchy();
}

ZstStageSession::~ZstStageSession()
{
	m_client_socket_index.clear();
	delete m_hierarchy;
}

void ZstStageSession::init()
{
}

void ZstStageSession::destroy()
{
	ZstSession::destroy();
	for (auto c : m_cables) {
		destroy_cable(c);
	}

	hierarchy()->destroy();
}

void ZstStageSession::on_receive_msg(ZstStageMessage * msg)
{
	ZstStageMessage * response = NULL;
	ZstPerformerStageProxy * sender = m_session->hierarchy()->find_entity(msg->sender());

	//Check client hasn't finished joining yet
	if (!sender)
		return;

	switch (msg->kind()) {
	case ZstMsgKind::CREATE_CABLE:
	{
		response = create_cable_handler(msg, sender);
		break;
	}

	case ZstMsgKind::DESTROY_CABLE:
	{
		response = destroy_cable_handler(msg);
		break;
	}

	case ZstMsgKind::SUBSCRIBE_TO_PERFORMER_ACK:
	{
		response = complete_client_connection_handler(msg, sender);
		break;
	}
	default:
		break;
	}

	if (response) {
		//Copy ID of the original message so we can match this message to a promise on the client
		//once upon a time there was a null pointer, it pointed to nothing.
		response->copy_id(msg);
		m_dispatch->send_to_address(response, sender);
	}
}

ZstStageMessage * ZstStageSession::create_cable_handler(ZstStageMessage * msg, ZstPerformer * performer)
{
	ZstStageMessage * response = msg_pool()->get_msg();

	//Unpack cable from message
	const ZstCable & cable = msg->unpack_payload_serialisable<ZstCable>(0);
	ZstLog::net(LogLevel::notification, "Received connect cable request for In:{} and Out:{}", cable.get_input_URI().path(), cable.get_output_URI().path());

	//Create cable 
	ZstCable * cable_ptr = create_cable(cable);

	if (!cable_ptr) {
		return response->init_message(ZstMsgKind::ERR_STAGE_BAD_CABLE_CONNECT_REQUEST);
	}

	//Create connection request for the entity who owns the input plug
	ZstPerformerStageProxy * input_performer = get_client(cable_ptr->get_input_URI());
	ZstPerformerStageProxy * output_performer = dynamic_cast<ZstPerformerStageProxy*>(get_client(cable_ptr->get_output_URI()));

	std::string id = std::string(msg->id(), ZSTMSG_UUID_LENGTH);

	//Check to see if one client is already connected to the other
	if (!output_performer->is_connected_to_subscriber_peer(input_performer))
	{
		m_promise_messages[id] = MessagePromise();
		MessageFuture future = m_promise_messages[id].get_future();

		//Create future action to run when the client responds
		future.then([this, cable_ptr, performer, id](MessageFuture f) {
			ZstMsgKind status(ZstMsgKind::EMPTY);
			try {
				ZstMsgKind status = f.get();
				if (status == ZstMsgKind::OK) {
					//Publish cable to graph
					create_cable_complete_handler(cable_ptr);

					//Let original requester know that their cable request has completed
					ZstStageMessage * ok_msg = msg_pool()->get_msg()->init_message(ZstMsgKind::OK);
					ok_msg->set_id(id.c_str());
					this->send_to_client(ok_msg, performer);
				}
				return status;
			}
			catch (const ZstTimeoutException & e) {
				ZstLog::net(LogLevel::error, "Client connection async response timed out - {}", e.what());
			}
			return status;
		});

		//Start the client connection
		connect_clients(id, output_performer, input_performer);
	}
	else {
		return create_cable_complete_handler(cable_ptr);
	}

	return NULL;
}

ZstStageMessage * ZstStageSession::create_cable_complete_handler(ZstCable * cable)
{
	ZstLog::net(LogLevel::notification, "Client connection complete. Publishing cable {}-{}", cable->get_input_URI().path(), cable->get_output_URI().path());
	publish_stage_update(msg_pool()->get_msg()->init_serialisable_message(ZstMsgKind::CREATE_CABLE, *cable));
	return msg_pool()->get_msg()->init_message(ZstMsgKind::OK);
}

ZstStageMessage * ZstStageSession::destroy_cable_handler(ZstStageMessage * msg)
{
	ZstStageMessage * response = msg_pool()->get_msg();
	const ZstCable & cable = msg->unpack_payload_serialisable<ZstCable>(0);
	ZstLog::net(LogLevel::notification, "Received destroy cable connection request");

	ZstCable * cable_ptr = get_cable_by_URI(cable.get_input_URI(), cable.get_output_URI());

	if (!destroy_cable(cable_ptr)) {
		response->init_message(ZstMsgKind::ERR_STAGE_BAD_CABLE_DISCONNECT_REQUEST);
	}

	return response->init_message(ZstMsgKind::OK);
}


// ---------

ZstPerformerStageProxy * ZstStageSession::get_client_from_socket_id(const std::string & socket_id)
{
	ZstPerformerStageProxy * performer = NULL;
	if (m_client_socket_index.find(socket_id) != m_client_socket_index.end()) {
		performer = m_client_socket_index[socket_id];
	}
	return performer;
}

std::string ZstStageSession::get_socket_ID(const ZstPerformer * performer)
{
	for (auto client : m_client_socket_index) {
		if (client.second == performer) {
			return client.first;
		}
	}
	return "";
}


// ------------


ZstCable * ZstStageSession::create_cable(const ZstCable & cable)
{
	return create_cable(cable.get_input_URI(), cable.get_output_URI());
}

ZstCable * ZstStageSession::create_cable(const ZstURI & input_URI, const ZstURI & output_URI)
{
	ZstCable * cable = NULL;
	cable = find_cable(input_URI, output_URI);

	//Check to see if we already have a cable
	if (cable != NULL) {
		ZstLog::net(LogLevel::warn, "Cable already exists");
		return NULL;
	}

	//Find target plugs, they shyould already exist on the graph
	int connect_status = 0;

	ZstPerformer * input_perf = get_client(input_URI);
	ZstPerformer * output_perf = get_client(output_URI);

	if (!input_perf || !output_perf) {
		ZstLog::net(LogLevel::notification, "Create cable could not find performer");
		return cable;
	}

	ZstInputPlug * input_plug = dynamic_cast<ZstInputPlug*>(input_perf->find_child_by_URI(input_URI));
	ZstOutputPlug * output_plug = dynamic_cast<ZstOutputPlug*>(output_perf->find_child_by_URI(output_URI));

	if (!input_plug || !output_plug) {
		ZstLog::net(LogLevel::notification, "Create cable could not find plugs");
		return cable;
	}

	//Verify plug directions are correct
	if (input_plug->direction() == ZstPlugDirection::OUT_JACK && output_plug->direction() == ZstPlugDirection::IN_JACK) {
		connect_status = 1;
	}
	else if (input_plug->direction() == ZstPlugDirection::IN_JACK && output_plug->direction() == ZstPlugDirection::OUT_JACK) {
		connect_status = 1;
	}
	else {
		ZstLog::net(LogLevel::notification, "Cable can't connect input to input or output to output");
		return cable;
	}

	//Finally create the cable
	cable = ZstCable::create(input_plug, output_plug);
	try {
		m_cables.insert(cable);
	}
	catch (std::exception e) {
		ZstLog::net(LogLevel::notification, "Couldn't insert cable. Reason:", e.what());
		ZstCable::destroy(cable);
		cable = NULL;
	}
	return cable;
}

int ZstStageSession::destroy_cable(const ZstURI & path) {
	int result = 1;
	bool fail = false;
	std::vector<ZstCable*> cables = find_cables(path);
	for (ZstCable * cable : cables) {
		if (!destroy_cable(cable))
		{
			fail = true;
		}
	}
	if (fail)
		result = 0;
	return result;
}

int ZstStageSession::destroy_cable(const ZstURI & input_plug, const ZstURI & output_plug) {
	return destroy_cable(get_cable_by_URI(input_plug, output_plug));
}

int ZstStageSession::destroy_cable(ZstCable * cable) {
	if (cable != NULL) {
		ZstLog::net(LogLevel::notification, "Destroying cable {} {}", cable->get_output_URI().path(), cable->get_input_URI().path());

		//Update rest of network
		publish_stage_update(msg_pool()->get_msg()->init_serialisable_message(ZstMsgKind::DESTROY_CABLE, *cable));

		m_cables.erase(cable);
		ZstCable::destroy(cable);
		cable = 0;
		return 1;
	}
	return 0;
}


void ZstStageSession::connect_clients(const std::string & response_id, ZstPerformerStageProxy * output_client, ZstPerformerStageProxy * input_client)
{
	ZstStageMessage * receiver_msg = m_dispatch->msg_pool()->get_msg()->init_message(ZstMsgKind::SUBSCRIBE_TO_PERFORMER);

	//Attach URI and IP of output client
	receiver_msg->append_str(output_client->URI().path(), output_client->URI().full_size());
	receiver_msg->append_str(output_client->ip_address().c_str(), strlen(output_client->ip_address().c_str()));
	receiver_msg->append_str(response_id.c_str(), response_id.size());

	ZstLog::net(LogLevel::notification, "Sending P2P subscribe request to {}", input_client->URI().path());
	m_dispatch->send_to_client(receiver_msg, input_client);

	ZstLog::net(LogLevel::notification, "Sending P2P handshake broadcast request to {}", output_client->URI().path());
	ZstStageMessage * broadcaster_msg = msg_pool()->get_msg()->init_message(ZstMsgKind::START_CONNECTION_HANDSHAKE);
	broadcaster_msg->append_str(input_client->URI().path(), input_client->URI().full_size());
	m_dispatch->send_to_client(broadcaster_msg, output_client);
}


ZstStageMessage * ZstStageSession::complete_client_connection_handler(ZstStageMessage * msg, ZstPerformerStageProxy * input_client)
{
	ZstPerformerStageProxy * output_client = get_client(ZstURI(msg->payload_at(0).data(), msg->payload_at(0).size()));
	if (output_client) {
		//Keep a record of which clients are connected to each other
		output_client->add_subscriber_peer(input_client);

		//Let the broadcaster know it can stop publishing messages
		ZstLog::net(LogLevel::notification, "Stopping P2P handshake broadcast from client {}", output_client->URI().path());
		ZstStageMessage * end_broadcast_msg = msg_pool()->get_msg()->init_message(ZstMsgKind::STOP_CONNECTION_HANDSHAKE);
		end_broadcast_msg->append_str(input_client->URI().path(), input_client->URI().full_size());
		send_to_client(end_broadcast_msg, output_client);
	}

	//Run any cable promises associated with this client connection
	std::string promise_id = std::string(msg->payload_at(1).data(), msg->payload_at(1).size());
	m_promise_messages.at(promise_id).set_value(ZstMsgKind::OK);
	/*const std::unordered_map<std::string, MessagePromise>::iterator future_it = m_promise_messages.find(promise_id);
	if (future_it != m_promise_messages.end()) {
	future_it->second.set_value(ZstMsgKind::OK);
	m_promise_messages.erase(future_it);
	}*/

	return msg_pool()->get_msg()->init_message(ZstMsgKind::OK);
}

ZstHierarchy * ZstStageSession::hierarchy()
{
	return m_hierarchy;
}
