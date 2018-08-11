#include "ZstStageHierarchy.h"
#include <boost/lexical_cast.hpp>

ZstStageHierarchy::~ZstStageHierarchy()
{
	m_client_socket_index.clear();
}

void ZstStageHierarchy::destroy() {
	for (auto p : m_clients) {
		destroy_client_handler(p.second);
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
		response = add_proxy_entity(msg->unpack_payload_serialisable<ZstComponent>());
		break;
	case ZstMsgKind::CREATE_CONTAINER:
		response = add_proxy_entity(msg->unpack_payload_serialisable<ZstContainer>());
		break;
	case ZstMsgKind::CREATE_PLUG:
		response = add_proxy_entity(msg->unpack_payload_serialisable<ZstPlug>());
		break;
	case ZstMsgKind::CREATE_ENTITY_FROM_TEMPLATE:
		response = create_entity_from_template_handler(msg);
		break;
	case ZstMsgKind::DESTROY_ENTITY:
		response = remove_proxy_entity(find_entity(ZstURI(msg->get_arg(ZstMsgArg::PATH))));
		break;
	default:
		break;
	}

	if (response != ZstMsgKind::EMPTY) {
		ZstMsgArgs args = {
			{ ZstMsgArg::SENDER_IDENTITY, sender_identity },
			{ ZstMsgArg::MSG_ID, boost::lexical_cast<std::string>(msg->id()) }
		};
		router_events().invoke([response, &args, &msg](ZstTransportAdaptor * adp) {
			adp->send_message(response, args);
		});
	}
}

ZstMsgKind ZstStageHierarchy::create_client_handler(std::string sender_identity, ZstMessage * msg)
{
	//Copy the id of the message so the sender will eventually match the response to a message promise
	ZstPerformer client = msg->unpack_payload_serialisable<ZstPerformer>();

	ZstLog::net(LogLevel::notification, "Registering new client {}", client.URI().path());

	//Only one client with this UUID at a time
	if (find_entity(client.URI())){
		ZstLog::net(LogLevel::warn, "Client already exists ", client.URI().path());
		return ZstMsgKind::ERR_STAGE_PERFORMER_ALREADY_EXISTS;
	}

	std::string reliable_address = "";
	std::string unreliable_address = "";
	try {
		reliable_address = msg->get_arg(ZstMsgArg::GRAPH_RELIABLE_OUTPUT_ADDRESS);
		unreliable_address = msg->get_arg(ZstMsgArg::GRAPH_UNRELIABLE_INPUT_ADDRESS);
	}
	catch (std::out_of_range) {
		ZstLog::net(LogLevel::warn, "Client sent message with unexpected argument");
	}

	//Copy streamable so we have a local ptr for the client
	ZstPerformerStageProxy * client_proxy = new ZstPerformerStageProxy(client, reliable_address, unreliable_address);
	assert(client_proxy);
	synchronisable_set_proxy(client_proxy);
	synchronisable_set_activation_status(client_proxy, ZstSyncStatus::ACTIVATED);
	client_proxy->add_adaptor(static_cast<ZstSynchronisableAdaptor*>(this));

	//Save our new client
	m_clients[client_proxy->URI()] = client_proxy;
	m_client_socket_index[std::string(sender_identity)] = client_proxy;

	//Update rest of network
	publisher_events().invoke([client_proxy](ZstTransportAdaptor * adp) {
		adp->send_message(ZstMsgKind::CREATE_PERFORMER, *client_proxy);
	});
	return ZstMsgKind::OK;
}

ZstMsgKind ZstStageHierarchy::destroy_client_handler(ZstPerformer * performer)
{
	//Nothing to do
	if (performer == NULL) {
		return ZstMsgKind::EMPTY;
	}

	ZstLog::net(LogLevel::notification, "Performer {} leaving", performer->URI().path());
	
	//Remove client and call all destructors in its hierarchy
	ZstPerformerMap::iterator client_it = m_clients.find(performer->URI());
	if (client_it != m_clients.end()) {
		m_clients.erase(client_it);
	}

	return remove_proxy_entity(performer);
}


ZstMsgKind ZstStageHierarchy::add_proxy_entity(const ZstEntityBase & entity)
{
	ZstLog::net(LogLevel::notification, "Registering new entity {}", entity.URI().path());

	ZstMsgKind msg_status = ZstHierarchy::add_proxy_entity(entity);
	ZstEntityBase * proxy = find_entity(entity.URI());
	
	//Update rest of network
	if (msg_status == ZstMsgKind::OK) {
		publisher_events().invoke([proxy, &entity](ZstTransportAdaptor * adp) {
			adp->send_message(ZstMessage::entity_kind(entity), *proxy);
		});
	}

	return msg_status;
}

ZstMsgKind ZstStageHierarchy::remove_proxy_entity(ZstEntityBase * entity)
{
	if (!entity)
		return ZstMsgKind::ERR_ENTITY_NOT_FOUND;

	//Update rest of network
	publisher_events().invoke([entity](ZstTransportAdaptor * adp) {
		adp->send_message(ZstMsgKind::DESTROY_ENTITY, { {ZstMsgArg::PATH, entity->URI().path()} });
	});

	ZstHierarchy::remove_proxy_entity(entity);
	
	return ZstMsgKind::OK;
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
