#include "ZstStageHierarchy.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>

using namespace boost::uuids;

ZstStageHierarchy::~ZstStageHierarchy()
{
	m_client_endpoint_UUIDS.clear();
    ZstEntityBundle bundle;
    for (auto p : get_performers(bundle)) {
        destroy_client_handler(dynamic_cast<ZstPerformer*>(p));
    }
}

void ZstStageHierarchy::destroy() {
	
	ZstHierarchy::destroy();
}

void ZstStageHierarchy::set_wake_condition(std::weak_ptr<ZstSemaphore> condition)
{
    ZstStageModule::set_wake_condition(condition);
    hierarchy_events()->set_wake_condition(condition);
    synchronisable_events()->set_wake_condition(condition);
}

ZstPerformer * ZstStageHierarchy::get_local_performer() const {
    return NULL;
}

void ZstStageHierarchy::process_events()
{
	ZstStageModule::process_events();
	ZstHierarchy::process_events();
}

void ZstStageHierarchy::on_receive_msg(ZstMessage * msg)
{
	ZstStageMessage * stage_msg = static_cast<ZstStageMessage*>(msg);
	ZstMsgKind response(ZstMsgKind::EMPTY);

	uuid sender_identity = stage_msg->endpoint_UUID();
	ZstPerformerStageProxy * sender = get_client_from_endpoint_UUID(sender_identity);

	switch (stage_msg->kind()) {
	case ZstMsgKind::CLIENT_LEAVING:
		response = destroy_client_handler(get_client_from_endpoint_UUID(sender_identity));
		break;
	case ZstMsgKind::CLIENT_JOIN:
		response = create_client_handler(stage_msg);
		break;
	case ZstMsgKind::CREATE_COMPONENT:
		response = add_proxy_entity(stage_msg->unpack_payload_serialisable<ZstComponent>(), stage_msg->id(), sender);
		break;
	case ZstMsgKind::CREATE_PLUG:
		response = add_proxy_entity(stage_msg->unpack_payload_serialisable<ZstPlug>(), stage_msg->id(), sender);
		break;
	case ZstMsgKind::CREATE_FACTORY:
		response = add_proxy_entity(stage_msg->unpack_payload_serialisable<ZstEntityFactory>(), stage_msg->id(), sender);
		break;
	case ZstMsgKind::UPDATE_ENTITY:
		response = update_proxy_entity(stage_msg->unpack_payload_serialisable<ZstEntityFactory>(), stage_msg->id());
		break;
	case ZstMsgKind::CREATE_ENTITY_FROM_FACTORY:
		response = create_entity_from_factory_handler(stage_msg, sender);
		break;
	case ZstMsgKind::DESTROY_ENTITY:
	{
		std::string path_str = stage_msg->get_arg<std::string>(ZstMsgArg::PATH);
		response = remove_proxy_entity(find_entity(ZstURI(path_str.c_str(), path_str.size())));
		break;
	}
	default:
		break;
	}

	if (response != ZstMsgKind::EMPTY) {
		ZstTransportArgs args;
		args.target_endpoint_UUID = sender_identity;
		args.msg_args = {{ get_msg_arg_name(ZstMsgArg::MSG_ID), stage_msg->id() }};
		router_events()->defer([response, args](std::shared_ptr<ZstTransportAdaptor> adaptor) {
			adaptor->send_msg(response, args);
		});
	}
}

ZstMsgKind ZstStageHierarchy::create_client_handler(ZstStageMessage * msg)
{
	//Copy the id of the message so the sender will eventually match the response to a message promise
	ZstPerformer client = msg->unpack_payload_serialisable<ZstPerformer>();

	ZstLog::server(LogLevel::notification, "Registering new client {}", client.URI().path());

	//Only one client with this UUID at a time
	if (find_entity(client.URI())){
		ZstLog::server(LogLevel::warn, "Client already exists ", client.URI().path());
		return ZstMsgKind::ERR_STAGE_PERFORMER_ALREADY_EXISTS;
	}

	std::string reliable_address = "";
	std::string unreliable_address = "";
	try {
		reliable_address = msg->get_arg<std::string>(ZstMsgArg::GRAPH_RELIABLE_OUTPUT_ADDRESS);
	}
	catch (nlohmann::detail::out_of_range) {
		ZstLog::server(LogLevel::warn, "No reliable graph address found in performer");
		//return ZstMsgKind::ERR_STAGE_REQUEST_MISSING_ARG;
	}

	try {
		unreliable_address = msg->get_arg<std::string>(ZstMsgArg::GRAPH_UNRELIABLE_INPUT_ADDRESS);
	}
	catch (nlohmann::detail::out_of_range) {
		ZstLog::server(LogLevel::debug, "No unreliable graph address found in performer");
	}

	//Copy streamable so we have a local ptr for the client
	ZstPerformerStageProxy * client_proxy = new ZstPerformerStageProxy(client, reliable_address, unreliable_address);
	synchronisable_set_proxy(client_proxy);
	synchronisable_set_activation_status(client_proxy, ZstSyncStatus::ACTIVATED);
	client_proxy->add_adaptor(ZstSynchronisableAdaptor::downcasted_shared_from_this<ZstSynchronisableAdaptor>());

	//Save our new client
	m_clients[client_proxy->URI()] = client_proxy;
	m_client_endpoint_UUIDS[msg->endpoint_UUID()] = client_proxy;
	
	//Cache new client and its contents
	ZstEntityBundle bundle;
	client_proxy->get_child_entities(bundle, true);
	ZstLog::server(LogLevel::debug, "New performer {} contains:", client_proxy->URI().path());
	for (auto c : bundle) {
		add_entity_to_lookup(c);
		ZstLog::server(LogLevel::debug, " - Entity: {}", c->URI().path());
	}

	//Cache factories
	bundle.clear();
	client_proxy->get_factories(bundle);
	for (auto f : bundle) {
		add_entity_to_lookup(f);
		ZstLog::server(LogLevel::debug, " - Factory: {}", f->URI().path());
		ZstURIBundle creatables;
		for (auto c : static_cast<ZstEntityFactory*>(f)->get_creatables(creatables)) {
			ZstLog::server(LogLevel::debug, "   - Creatable: {}", c.path());
		}
	}

	//Update rest of network
	ZstTransportArgs args;
	client_proxy->write_json(args.msg_payload);
	broadcast_message(ZstMsgKind::CREATE_PERFORMER, args);

	return Signal_OK;
}

ZstMsgKind ZstStageHierarchy::destroy_client_handler(ZstPerformer * performer)
{
	//Nothing to do
	if (performer == NULL) {
		return ZstMsgKind::EMPTY;
	}

	ZstLog::server(LogLevel::notification, "Performer {} leaving", performer->URI().path());
	
	//Remove client and call all destructors in its hierarchy
	ZstPerformerMap::iterator client_it = m_clients.find(performer->URI());
	if (client_it != m_clients.end()) {
		m_clients.erase(client_it);
	}

	//Remove entity and children from lookup
	ZstEntityBundle bundle;
    performer->get_child_entities(bundle, true);
	for (auto c : bundle) {
		remove_entity_from_lookup(c);
	}

	return remove_proxy_entity(performer);
}

void ZstStageHierarchy::broadcast_message(const ZstMsgKind & msg_kind, const ZstTransportArgs & args)
{
	ZstEntityBundle bundle;
	for (auto entity : get_performers(bundle)) 
	{
		//Can only send messages to performers
		ZstPerformer * performer = dynamic_cast<ZstPerformerStageProxy*>(entity);
		if (!performer) {
			ZstLog::server(LogLevel::error, "Not a performer");
			continue;
		}

		//Send message to client
		whisper_message(performer, msg_kind, args);
	}
}

void ZstStageHierarchy::whisper_message(ZstPerformer* performer, const ZstMsgKind& msg_kind, const ZstTransportArgs& args)
{
	ZstTransportArgs endpoint_args = args;
	endpoint_args.target_endpoint_UUID = get_endpoint_UUID_from_client(performer);
	router_events()->defer([this, msg_kind, endpoint_args, performer](std::shared_ptr<ZstTransportAdaptor> adaptor) {
		adaptor->send_msg(msg_kind, endpoint_args);
	});
}

ZstMsgKind ZstStageHierarchy::add_proxy_entity(const ZstEntityBase & entity, ZstMsgID request_ID, ZstPerformer * sender)
{
	ZstLog::server(LogLevel::notification, "Registering new proxy entity {}", entity.URI().path());

	if (sender->URI().first() != entity.URI().first()) {
		//A performer is requesting this entity be attached to another performer
		ZstLog::server(LogLevel::warn, "TODO: Performer requesting new entity to be attached to another performer", entity.URI().path());
		return ZstMsgKind::ERR_ENTITY_NOT_FOUND;
	}

	ZstMsgKind msg_status = ZstHierarchy::add_proxy_entity(entity);
	ZstEntityBase * proxy = find_entity(entity.URI());
	if (!proxy) {
		ZstLog::server(LogLevel::warn, "No proxy entity found");
		return ZstMsgKind::ERR_ENTITY_NOT_FOUND;
	}
    
    //Dispatch internal module events
    dispatch_entity_arrived_event(proxy);
	
	//Update rest of network
	ZstTransportArgs args;
	args.msg_ID = request_ID;
	proxy->write_json(args.msg_payload);
	broadcast_message(ZstStageMessage::entity_kind(entity), args);

	return msg_status;
}

ZstMsgKind ZstStageHierarchy::update_proxy_entity(const ZstEntityBase & entity, ZstMsgID request_ID)
{
	ZstLog::server(LogLevel::notification, "Updating proxy entity {}", entity.URI().path());
	ZstMsgKind msg_status = ZstHierarchy::update_proxy_entity(entity);

	//Update rest of network
	if (msg_status == Signal_OK) {
		ZstTransportArgs args;
		args.msg_ID = request_ID;
		entity.write_json(args.msg_payload);
		broadcast_message(ZstMsgKind::UPDATE_ENTITY, args);
	}

	//Updating entities is a publish action
	return ZstMsgKind::EMPTY;
}

ZstMsgKind ZstStageHierarchy::remove_proxy_entity(ZstEntityBase * entity)
{
	if (!entity)
		return ZstMsgKind::ERR_ENTITY_NOT_FOUND;

	ZstLog::server(LogLevel::notification, "Removing proxy entity {}", entity->URI().path());

	//Update rest of network
	ZstTransportArgs args;
	args.msg_args = { {get_msg_arg_name(ZstMsgArg::PATH), entity->URI().path()} };
	broadcast_message(ZstMsgKind::DESTROY_ENTITY, args);

	//Finally, remove the entity
	ZstHierarchy::remove_proxy_entity(entity);
	
	return Signal_OK;
}

ZstMsgKind ZstStageHierarchy::create_entity_from_factory_handler(ZstStageMessage * msg, ZstPerformerStageProxy * sender)
{
	//First, find the factory
	std::string factory_path_str = msg->get_arg<std::string>(ZstMsgArg::PATH);
	ZstURI factory_path = ZstURI(factory_path_str.c_str(), factory_path_str.size()).parent();

	ZstLog::server(LogLevel::notification, "Forwarding creatable entity request {} with id {}", factory_path.path(), msg->id());

	ZstEntityFactory * factory = dynamic_cast<ZstEntityFactory*>(find_entity(factory_path));
	if (!factory) {
		ZstLog::server(LogLevel::error, "Could not find factory {}", factory_path.path());
		return ZstMsgKind::ERR_ENTITY_NOT_FOUND;
	}

	//Find performer who owns the factory
	ZstPerformerStageProxy * factory_performer = dynamic_cast<ZstPerformerStageProxy*>(find_entity(factory_path.first()));

	//Check to see if one client is already connected to the other
	if (!factory_performer)
	{
		ZstLog::server(LogLevel::error, "Could not find factory {}", factory_path.path());
		return ZstMsgKind::ERR_STAGE_PERFORMER_NOT_FOUND;
	}

	//Send creatable message to the performer that owns the factory
	ZstMsgID id = msg->id();
	ZstTransportArgs args;
	args.msg_args = {
		{ get_msg_arg_name(ZstMsgArg::NAME) , msg->get_arg<std::string>(ZstMsgArg::NAME) },
		{ get_msg_arg_name(ZstMsgArg::PATH) , msg->get_arg<std::string>(ZstMsgArg::PATH) }
		//{ get_msg_arg_name(ZstMsgArg::MSG_ID), id}
	};
	args.msg_send_behaviour = ZstTransportRequestBehaviour::ASYNC_REPLY;
	args.on_recv_response = [this, factory_performer, sender, factory_path, id](ZstMessageReceipt receipt){
		if (receipt.status == ZstMsgKind::ERR_ENTITY_NOT_FOUND) {
			ZstLog::server(LogLevel::error, "Creatable request failed at origin with status {}", get_msg_name(receipt.status));
			return;
		}
		ZstLog::server(LogLevel::notification, "Remote factory created entity {}", factory_path.path());

		//Ack response
		ZstTransportArgs create_args;
		create_args.msg_args = { {get_msg_arg_name(ZstMsgArg::MSG_ID), id} };
		whisper_message(sender, Signal_OK, create_args);
	};

	//Send 
	whisper_message(factory_performer, msg->kind(), args);

	return ZstMsgKind::EMPTY;
}

ZstPerformerStageProxy * ZstStageHierarchy::get_client_from_endpoint_UUID(const uuid & endpoint_UUID)
{
	try { 
		return  m_client_endpoint_UUIDS.at(endpoint_UUID);
	} catch (std::out_of_range) {}
	return NULL;
}

uuid ZstStageHierarchy::get_endpoint_UUID_from_client(const ZstPerformer * performer)
{
	for (auto client : m_client_endpoint_UUIDS) {
		if (client.second == performer) {
			return client.first;
		}
	}
	return nil_generator()();
}
