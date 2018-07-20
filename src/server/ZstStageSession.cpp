#include <boost/lexical_cast.hpp>
#include "ZstStageSession.h"


ZstStageSession::ZstStageSession() : m_hierarchy(NULL)
{
	m_hierarchy = new ZstStageHierarchy();
}

ZstStageSession::~ZstStageSession()
{
	delete m_hierarchy;
}

void ZstStageSession::init()
{
	//Attach this as an adaptor to the hierarchy module to handle hierarchy events that will modify cables
	m_hierarchy->hierarchy_events().add_adaptor(this);
}

void ZstStageSession::destroy()
{
	ZstSession::destroy();
	for (auto c : m_cables) {
		destroy_cable(c);
	}

	hierarchy()->destroy();
}

void ZstStageSession::on_receive_msg(ZstMessage * msg)
{
	ZstMsgKind response(ZstMsgKind::EMPTY);
	std::string sender_identity = msg->get_arg(ZstMsgArg::SENDER_IDENTITY);
	ZstPerformerStageProxy * sender = m_hierarchy->get_client_from_socket_id(sender_identity);

	//Check client hasn't finished joining yet
	if (!sender)
		return;

	switch (msg->kind()) {
	case ZstMsgKind::CLIENT_SYNC:
		response = synchronise_client_graph(sender);
		break;
	case ZstMsgKind::CREATE_CABLE:
		response = create_cable_handler(msg, sender);
		break;
	case ZstMsgKind::DESTROY_CABLE:
		response = destroy_cable_handler(msg);
		break;
	case ZstMsgKind::SUBSCRIBE_TO_PERFORMER_ACK:
		response = complete_client_connection_handler(msg, sender);
		break;
	case ZstMsgKind::CLIENT_HEARTBEAT:
		sender->set_heartbeat_active();
		response = ZstMsgKind::OK;
		break;
	default:
		break;
	}

	if (response != ZstMsgKind::EMPTY) {
		m_router_events.invoke([response, &sender_identity, &msg](ZstTransportAdaptor * adp) {
			adp->send_message(response, {
				{ ZstMsgArg::SENDER_IDENTITY, sender_identity },
				{ ZstMsgArg::MSG_ID, std::string((char*)msg->id(), sizeof(ZstMsgKind)) }
				});
		});
	}
}

ZstMsgKind ZstStageSession::synchronise_client_graph(ZstPerformer * client) {

	ZstLog::net(LogLevel::notification, "Sending graph snapshot to {}", client->URI().path());

	//Pack performer root entities
	std::vector<ZstPerformer*> performers = hierarchy()->get_performers();
	for (auto performer : performers) {
		//Only pack performers that aren't the destination client
		if (performer->URI() != client->URI()) {
			m_router_events.invoke([performer](ZstTransportAdaptor * adp) {
				adp->send_message(ZstMsgKind::CREATE_PERFORMER, *performer);
			});
		}
	}

	//Pack cables
	for (auto cable : m_cables) {
		m_router_events.invoke([cable](ZstTransportAdaptor * adp) {
			adp->send_message(ZstMsgKind::CREATE_CABLE, *cable);
		});
	}

	return ZstMsgKind::OK;
}

ZstMsgKind ZstStageSession::create_cable_handler(ZstMessage * msg, ZstPerformer * performer)
{
	//Unpack cable from message
	ZstCable cable = msg->unpack_payload_serialisable<ZstCable>(0);
	ZstLog::net(LogLevel::notification, "Received connect cable request for In:{} and Out:{}", cable.get_input_URI().path(), cable.get_output_URI().path());

	//Create cable 
	if (find_cable(cable.get_input_URI(), cable.get_output_URI())){
		ZstLog::net(LogLevel::warn, "Cable already exists");
		return ZstMsgKind::ERR_STAGE_BAD_CABLE_CONNECT_REQUEST;
	}

	ZstCable * cable_ptr = create_cable(cable);

	if (!cable_ptr) {
		return ZstMsgKind::ERR_STAGE_BAD_CABLE_CONNECT_REQUEST;
	}

	//Create connection request for the entity who owns the input plug
	ZstPerformerStageProxy * input_performer = dynamic_cast<ZstPerformerStageProxy*>(hierarchy()->find_entity(cable_ptr->get_input_URI().first()));
	ZstPerformerStageProxy * output_performer = dynamic_cast<ZstPerformerStageProxy*>(hierarchy()->find_entity(cable_ptr->get_output_URI().first()));

	//Check to see if one client is already connected to the other
	if (!output_performer->is_connected_to_subscriber_peer(input_performer))
	{
		ZstMsgID id = msg->id();
		m_deferred_cables[id] = MessagePromise();
		MessageFuture future = m_deferred_cables[id].get_future();

		//Create future action to run when the client responds
		future.then([this, cable_ptr, performer, id](MessageFuture f) {
			ZstMsgKind status(ZstMsgKind::EMPTY);
			try {
				ZstMsgKind status = f.get();
				if (status == ZstMsgKind::OK) {
					//Publish cable to graph
					create_cable_complete_handler(cable_ptr);

					m_router_events.invoke([id, this, performer](ZstTransportAdaptor * adp) {
						adp->send_message(ZstMsgKind::OK, { 
							{ZstMsgArg::SENDER_IDENTITY, this->m_hierarchy->get_socket_ID(performer)},
							{ZstMsgArg::MSG_ID, std::string((char*)id, sizeof(ZstMsgKind)) }
						});
					});
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

	return ZstMsgKind::EMPTY;
}

ZstMsgKind ZstStageSession::create_cable_complete_handler(ZstCable * cable)
{
	ZstLog::net(LogLevel::notification, "Client connection complete. Publishing cable {}-{}", cable->get_input_URI().path(), cable->get_output_URI().path());
	m_router_events.invoke([cable](ZstTransportAdaptor * adp) {
		adp->send_message(ZstMsgKind::CREATE_CABLE, *cable);
	});
	return ZstMsgKind::OK;
}

ZstMsgKind ZstStageSession::destroy_cable_handler(ZstMessage * msg)
{
	const ZstCable & cable = msg->unpack_payload_serialisable<ZstCable>(0);
	ZstLog::net(LogLevel::notification, "Received destroy cable connection request");

	ZstCable * cable_ptr = find_cable(cable.get_input_URI(), cable.get_output_URI());
	destroy_cable(cable_ptr);

	return ZstMsgKind::OK;
}

void ZstStageSession::on_performer_leaving(ZstPerformer * performer)
{
	disconnect_cables(performer);
}

void ZstStageSession::on_entity_leaving(ZstEntityBase * entity)
{
	disconnect_cables(entity);
}

void ZstStageSession::on_plug_leaving(ZstPlug * plug)
{
	disconnect_cables(plug);
}

void ZstStageSession::disconnect_cables(ZstEntityBase * entity)
{
	ZstCableBundle * bundle = entity->acquire_cable_bundle();
	for (auto c : *bundle) {
		destroy_cable(c);
	}
	entity->release_cable_bundle(bundle);
}

void ZstStageSession::destroy_cable(ZstCable * cable) {
	if (!cable)
		return;

	ZstLog::net(LogLevel::notification, "Destroying cable {} {}", cable->get_output_URI().path(), cable->get_input_URI().path());
	ZstSession::destroy_cable(cable, ZstTransportSendType::PUBLISH);

	//Update rest of network
	m_publisher_events.invoke([this, cable](ZstTransportAdaptor * adp) {adp->send_message(ZstMsgKind::DESTROY_CABLE, *cable); });
}


void ZstStageSession::connect_clients(const ZstMsgID & response_id, ZstPerformerStageProxy * output_client, ZstPerformerStageProxy * input_client)
{
	ZstLog::net(LogLevel::notification, "Sending P2P subscribe request to {}", input_client->URI().path());

	//Attach URI and IP of output client
	std::stringstream conn_s;
	conn_s << response_id;
	ZstMsgArgs receiver_args{
		{ ZstMsgArg::OUTPUT_PATH, output_client->URI().path() },
		{ ZstMsgArg::OUTPUT_ADDRESS, output_client->ip_address() },
		{ ZstMsgArg::CONNECTION_MSG_ID, conn_s.str() },
		{ ZstMsgArg::SENDER_IDENTITY, m_hierarchy->get_socket_ID(input_client) }
	};
	
	m_router_events.invoke([&receiver_args](ZstTransportAdaptor * adp) {
		adp->send_message(ZstMsgKind::SUBSCRIBE_TO_PERFORMER, receiver_args);
	});

	ZstLog::net(LogLevel::notification, "Sending P2P handshake broadcast request to {}", output_client->URI().path());
	ZstMsgArgs broadcaster_args{ {ZstMsgArg::SENDER_IDENTITY, m_hierarchy->get_socket_ID(input_client) } };
	m_router_events.invoke([&broadcaster_args](ZstTransportAdaptor * adp) {
		adp->send_message(ZstMsgKind::START_CONNECTION_HANDSHAKE, broadcaster_args);
	});
}


ZstMsgKind ZstStageSession::complete_client_connection_handler(ZstMessage * msg, ZstPerformerStageProxy * input_client)
{
	ZstPerformerStageProxy * output_client = dynamic_cast<ZstPerformerStageProxy*>(m_hierarchy->find_entity(ZstURI(msg->get_arg(ZstMsgArg::OUTPUT_PATH))));
	if (!output_client) {
		ZstLog::net(LogLevel::error, "Could not find client {}", msg->get_arg(ZstMsgArg::OUTPUT_PATH));
		return ZstMsgKind::ERR_STAGE_PERFORMER_NOT_FOUND;
	}

	//Keep a record of which clients are connected to each other
	output_client->add_subscriber_peer(input_client);

	//Let the broadcaster know it can stop publishing messages
	ZstLog::net(LogLevel::notification, "Stopping P2P handshake broadcast from client {}", output_client->URI().path());
	ZstMsgArgs args{ 
		{ZstMsgArg::INPUT_PATH, input_client->URI().path()},
		{ZstMsgArg::SENDER_IDENTITY, m_hierarchy->get_socket_ID(output_client)}
	};
	m_router_events.invoke([&args](ZstTransportAdaptor * adp) {
		adp->send_message(ZstMsgKind::STOP_CONNECTION_HANDSHAKE, args);
	});

	//Run any cable promises associated with this client connection
	ZstMsgID id = boost::lexical_cast<uint64_t>(msg->get_arg(ZstMsgArg::CONNECTION_MSG_ID));

	m_deferred_cables.at(id).set_value(ZstMsgKind::OK);
	m_deferred_cables.erase(id);
	return ZstMsgKind::OK;
}

ZstStageHierarchy * ZstStageSession::hierarchy()
{
	return m_hierarchy;
}
