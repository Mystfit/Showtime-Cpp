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
	ZstHierarchy::destroy();
}

void ZstStageHierarchy::on_receive_msg(ZstMessage * msg)
{
	ZstMsgKind response(ZstMsgKind::EMPTY);
	std::string sender_identity = msg->get_arg(ZstMsgArg::SENDER_IDENTITY);
	ZstPerformerStageProxy * sender = get_client_from_socket_id(sender_identity);

	switch (msg->kind()) {
	case ZstMsgKind::CLIENT_LEAVING:
		response = destroy_client_handler(get_client_from_socket_id(sender_identity));
		break;
	case ZstMsgKind::CLIENT_JOIN:
		response = create_client_handler(sender_identity, msg);
		break;
	case ZstMsgKind::CREATE_COMPONENT:
		response = add_proxy_entity(msg->unpack_payload_serialisable<ZstComponent>(), msg->id(), sender);
		break;
	case ZstMsgKind::CREATE_CONTAINER:
		response = add_proxy_entity(msg->unpack_payload_serialisable<ZstContainer>(), msg->id(), sender);
		break;
	case ZstMsgKind::CREATE_PLUG:
		response = add_proxy_entity(msg->unpack_payload_serialisable<ZstPlug>(), msg->id(), sender);
		break;
	case ZstMsgKind::CREATE_FACTORY:
		response = add_proxy_entity(msg->unpack_payload_serialisable<ZstEntityFactory>(), msg->id(), sender);
		break;
	case ZstMsgKind::UPDATE_ENTITY:
		response = update_proxy_entity(msg->unpack_payload_serialisable<ZstEntityFactory>(), msg->id());
		break;
	case ZstMsgKind::CREATE_ENTITY_FROM_FACTORY:
		response = create_entity_from_factory_handler(msg, sender);
		break;
	case ZstMsgKind::DESTROY_ENTITY:
		response = remove_proxy_entity(find_entity(ZstURI(msg->get_arg(ZstMsgArg::PATH))));
		break;
	default:
		break;
	}

	if (response != ZstMsgKind::EMPTY) {
		ZstMsgArgs args = {
			{ ZstMsgArg::DESTINATION_IDENTITY, sender_identity },
			{ ZstMsgArg::MSG_ID, boost::lexical_cast<std::string>(msg->id()) }
		};
		router_events().invoke([response, &args, &msg](ZstTransportAdaptor * adp) {
			adp->on_send_msg(response, args);
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
	
	//Cache new client and its contents
	ZstEntityBundle bundle;
	client_proxy->get_child_entities(bundle, true);
	ZstLog::net(LogLevel::debug, "New performer {} contains:", client_proxy->URI().path());
	for (auto c : bundle) {
		add_entity_to_lookup(c);
		ZstLog::net(LogLevel::debug, " - Entity: {}", c->URI().path());
	}

	//Cache factories
	bundle.clear();
	client_proxy->get_factories(bundle);
	for (auto f : bundle) {
		add_entity_to_lookup(f);
		ZstLog::net(LogLevel::debug, " - Factory: {}", f->URI().path());
		ZstURIBundle creatables;
		for (auto c : static_cast<ZstEntityFactory*>(f)->get_creatables(creatables)) {
			ZstLog::net(LogLevel::debug, "   - Creatable: {}", c.path());
		}
	}

	//Update rest of network
	publisher_events().invoke([&client_proxy](ZstTransportAdaptor * adp) {
		adp->on_send_msg(ZstMsgKind::CREATE_PERFORMER, client_proxy->as_json_str());
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

	//Add entity and children to lookup
	ZstEntityBundle bundle;
	for (auto c : performer->get_child_entities(bundle, true)) {
		remove_entity_from_lookup(c);
	}

	return remove_proxy_entity(performer);
}


ZstMsgKind ZstStageHierarchy::add_proxy_entity(const ZstEntityBase & entity, ZstMsgID request_ID, ZstPerformer * sender)
{
	ZstLog::net(LogLevel::notification, "Registering new proxy entity {}", entity.URI().path());

	ZstMsgKind msg_status = ZstHierarchy::add_proxy_entity(entity);
	ZstEntityBase * proxy = find_entity(entity.URI());
	if (!proxy) {
		ZstLog::net(LogLevel::warn, "No proxy entity found");
		return ZstMsgKind::ERR_ENTITY_NOT_FOUND;
	}

	if (sender->URI().first() != proxy->URI().first()) {
		//A performer is requesting this entity be attached to another performer
	}
	
	//Update rest of network
	if (msg_status == ZstMsgKind::OK) {
		ZstMsgArgs args{ {ZstMsgArg::MSG_ID, boost::lexical_cast<std::string>(request_ID)} };
		publisher_events().invoke([&proxy, &entity, &args](ZstTransportAdaptor * adp) {
			adp->on_send_msg(ZstMessage::entity_kind(entity), args, proxy->as_json_str());
		});
	}

	return msg_status;
}

ZstMsgKind ZstStageHierarchy::update_proxy_entity(const ZstEntityBase & entity, ZstMsgID request_ID)
{
	ZstLog::net(LogLevel::notification, "Updating proxy entity {}", entity.URI().path());
	ZstMsgKind msg_status = ZstHierarchy::update_proxy_entity(entity);

	//Update rest of network
	if (msg_status == ZstMsgKind::OK) {
		ZstMsgArgs args{ { ZstMsgArg::MSG_ID, boost::lexical_cast<std::string>(request_ID) } };
		publisher_events().invoke([&entity, &args](ZstTransportAdaptor * adp) {
			adp->on_send_msg(ZstMsgKind::UPDATE_ENTITY, args, entity.as_json_str());
		});
	}

	//Updating entities is a publish action
	return ZstMsgKind::EMPTY;
}

ZstMsgKind ZstStageHierarchy::remove_proxy_entity(ZstEntityBase * entity)
{
	if (!entity)
		return ZstMsgKind::ERR_ENTITY_NOT_FOUND;

	ZstLog::net(LogLevel::notification, "Removing proxy entity {}", entity->URI().path());

	//Update rest of network
	publisher_events().invoke([entity](ZstTransportAdaptor * adp) {
		adp->on_send_msg(ZstMsgKind::DESTROY_ENTITY, { {ZstMsgArg::PATH, entity->URI().path()} });
	});

	ZstHierarchy::remove_proxy_entity(entity);
	
	return ZstMsgKind::OK;
}

ZstMsgKind ZstStageHierarchy::create_entity_from_factory_handler(ZstMessage * msg, ZstPerformerStageProxy * sender)
{
	//First, find the factory
	ZstURI factory_path = ZstURI(msg->get_arg(ZstMsgArg::PATH)).parent();

	ZstLog::net(LogLevel::notification, "Forwarding creatable entity request {} with id {}", factory_path.path(), msg->id());

	ZstEntityFactory * factory = dynamic_cast<ZstEntityFactory*>(find_entity(factory_path));
	if (!factory) {
		ZstLog::net(LogLevel::error, "Could not find factory {}", factory_path.path());
		ZstMsgKind::ERR_ENTITY_NOT_FOUND;
	}

	//Find performer who owns the factory
	ZstPerformerStageProxy * factory_performer = dynamic_cast<ZstPerformerStageProxy*>(find_entity(factory_path.first()));

	//Check to see if one client is already connected to the other
	if (!factory_performer)
	{
		ZstLog::net(LogLevel::error, "Could not find factory {}", factory_path.path());
		return ZstMsgKind::ERR_STAGE_PERFORMER_NOT_FOUND;
	}

	std::string request_id = boost::lexical_cast<std::string>(msg->id());

	//Send creatable message to the performer that owns the factory
	ZstMsgArgs args{ 
		{ ZstMsgArg::NAME , msg->get_arg(ZstMsgArg::NAME) },
		{ ZstMsgArg::PATH , msg->get_arg(ZstMsgArg::PATH)},
		{ ZstMsgArg::DESTINATION_IDENTITY, get_socket_ID(factory_performer) },
		{ ZstMsgArg::MSG_ID, request_id}
	};
	router_events().invoke([this, msg, request_id, &args, factory_path, sender](ZstTransportAdaptor * adp)
	{
		adp->on_send_msg(msg->kind(), ZstTransportSendType::ASYNC_REPLY, args, [this, msg, request_id, factory_path, sender](ZstMessageReceipt receipt)
		{
			if (receipt.status == ZstMsgKind::ERR_ENTITY_NOT_FOUND) {
				ZstLog::net(LogLevel::error, "Creatable request failed at origin with status {}", ZstMsgNames[receipt.status]);
				return;
			}
			ZstLog::net(LogLevel::notification, "Remote factory created entity {}", factory_path.path());
		}); 
	});

	return ZstMsgKind::EMPTY;
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
