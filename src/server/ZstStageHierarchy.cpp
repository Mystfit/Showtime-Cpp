#include "ZstStageHierarchy.h"

ZstStageHierarchy::~ZstStageHierarchy()
{
	m_client_socket_index.clear();
}

void ZstStageHierarchy::destroy() {
	for (auto p : m_clients) {
		destroy_client(p.second);
	}
}

void ZstStageHierarchy::on_receive_msg(ZstMessage * msg)
{
	ZstMsgKind response(ZstMsgKind::EMPTY);
	std::string sender_identity = msg->get_arg(ZstMsgArg::SENDER_IDENTITY);

	switch (msg->kind()) {
	case ZstMsgKind::CLIENT_LEAVING:
		response = destroy_client_handler(get_client_from_socket_id(sender_identity));
		break;
	case ZstMsgKind::CLIENT_JOIN:
		response = create_client_handler(sender_identity, msg);
		break;
	case ZstMsgKind::CREATE_COMPONENT:
		add_proxy_entity(msg->unpack_payload_serialisable<ZstComponent>(0)) ? ZstMsgKind::OK : ZstMsgKind::ERR_ENTITY_NOT_FOUND;
		break;
	case ZstMsgKind::CREATE_CONTAINER:
		add_proxy_entity(msg->unpack_payload_serialisable<ZstContainer>(0));
		break;
	case ZstMsgKind::CREATE_PLUG:
		add_proxy_entity(msg->unpack_payload_serialisable<ZstPlug>(0));
		break;
	case ZstMsgKind::CREATE_ENTITY_FROM_TEMPLATE:
		response = create_entity_from_template_handler(msg);
		break;
	case ZstMsgKind::DESTROY_ENTITY:
		remove_proxy_entity(find_entity(ZstURI(msg->get_arg(ZstMsgArg::PATH))));
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

void ZstStageHierarchy::destroy_client(ZstPerformer * performer)
{
	//Nothing to do
	if (performer == NULL) {
		return;
	}

	ZstLog::net(LogLevel::notification, "Performer {} leaving", performer->URI().path());

	ZstHierarchy::destroy_entity(performer);
	hierarchy_events().invoke([performer](ZstHierarchyAdaptor * adp) { adp->on_performer_leaving(performer); });
	
	//Remove client and call all destructors in its hierarchy
	ZstPerformerMap::iterator client_it = m_clients.find(performer->URI());
	if (client_it != m_clients.end()) {
		m_clients.erase(client_it);
	}

	m_publisher_events.invoke([performer](ZstTransportAdaptor * adp) {
		adp->send_message(ZstMsgKind::DESTROY_ENTITY, { {ZstMsgArg::PATH, performer->URI().path() } });
	});

	delete performer;
}

ZstMsgKind ZstStageHierarchy::create_client_handler(std::string sender_identity, ZstMessage * msg)
{
	//Copy the id of the message so the sender will eventually match the response to a message promise
	ZstPerformer client = msg->unpack_payload_serialisable<ZstPerformer>(0);

	ZstLog::net(LogLevel::notification, "Registering new client {}", client.URI().path());

	//Only one client with this UUID at a time
	if (find_entity(client.URI())){
		ZstLog::net(LogLevel::warn, "Client already exists ", client.URI().path());
		return ZstMsgKind::ERR_STAGE_PERFORMER_ALREADY_EXISTS;
	}

	std::string ip_address = "";
	try {
		ip_address = msg->get_arg(ZstMsgArg::OUTPUT_ADDRESS);
	}
	catch (std::out_of_range) {
		ZstLog::net(LogLevel::warn, "Client sent message with unexpected argument");
	}

	//Copy streamable so we have a local ptr for the client
	ZstPerformerStageProxy * client_proxy = new ZstPerformerStageProxy(client, ip_address);
	assert(client_proxy);

	//Save our new client
	//add_performer(client);
	m_clients[client_proxy->URI()] = client_proxy;
	m_client_socket_index[std::string(sender_identity)] = client_proxy;

	//Update rest of network
	m_publisher_events.invoke([client_proxy](ZstTransportAdaptor * adp) {
		adp->send_message(ZstMsgKind::CREATE_PERFORMER, *client_proxy);
	});
	return ZstMsgKind::OK;
}

ZstMsgKind ZstStageHierarchy::destroy_client_handler(ZstPerformer * performer)
{
	destroy_client(performer);
	return ZstMsgKind::OK;
}


ZstMsgKind ZstStageHierarchy::add_proxy_entity(ZstEntityBase & entity)
{
	ZstLog::net(LogLevel::notification, "Registering new entity {}", entity.URI().path());

	ZstMsgKind msg_status = ZstHierarchy::add_proxy_entity(entity);
	ZstEntityBase * proxy = find_entity(entity.URI());

	//Update rest of network
	m_publisher_events.invoke([proxy](ZstTransportAdaptor * adp) {
		adp->send_message(ZstMsgKind::CREATE_PERFORMER, *proxy);
	});
	return msg_status;
}

void ZstStageHierarchy::remove_proxy_entity(ZstEntityBase * entity)
{
	ZstHierarchy::remove_proxy_entity(entity);

	//ZstMsgKind::ERR_ENTITY_NOT_FOUND;

	//Let session remove all cables linked to this entity
	if (strcmp(entity->entity_type(), PERFORMER_TYPE) == 0) {
		hierarchy_events().invoke([entity](ZstHierarchyAdaptor *adp) {adp->on_entity_leaving(entity); });
	}
	
	//Update rest of network
	m_publisher_events.invoke([entity](ZstTransportAdaptor * adp) {
		adp->send_message(ZstMsgKind::DESTROY_ENTITY, { {ZstMsgArg::PATH, entity->URI().path()} });
	});

	delete entity;
}


ZstMsgKind ZstStageHierarchy::create_entity_template_handler(ZstMessage * msg)
{
	//TODO: Implement this
	throw std::logic_error("Creating entity types not implemented yet");
	return ZstMsgKind::ERR_ENTITY_NOT_FOUND;
}


ZstMsgKind ZstStageHierarchy::create_entity_from_template_handler(ZstMessage * msg)
{
	throw std::logic_error("Creating entity types not implemented yet");
	return ZstMsgKind::ERR_ENTITY_NOT_FOUND;
}

ZstPerformerStageProxy * ZstStageHierarchy::get_client_from_socket_id(const std::string & socket_id)
{
	ZstPerformerStageProxy * performer = NULL;
	if (m_client_socket_index.find(socket_id) != m_client_socket_index.end()) {
		performer = m_client_socket_index[socket_id];
	}
	return performer;
}

std::string ZstStageHierarchy::get_socket_ID(const ZstPerformer * performer)
{
	for (auto client : m_client_socket_index) {
		if (client.second == performer) {
			return client.first;
		}
	}
	return "";
}
