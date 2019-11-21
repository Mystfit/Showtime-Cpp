#include "ZstStageHierarchy.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>

using namespace boost::uuids;
using namespace flatbuffers;

namespace showtime {

ZstStageHierarchy::~ZstStageHierarchy()
{
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

ZstPerformer* ZstStageHierarchy::get_local_performer() const {
	return NULL;
}

void ZstStageHierarchy::process_events()
{
	ZstStageModule::process_events();
	ZstHierarchy::process_events();
}

void ZstStageHierarchy::on_receive_msg(std::shared_ptr<ZstStageMessage> msg)
{
	Signal response = Signal_EMPTY;
	ZstPerformerStageProxy* sender = get_client_from_endpoint_UUID(msg->endpoint_UUID());

	switch (msg->type()) {
	case Content_SignalMessage:
		response = signal_handler(msg->buffer()->content_as_SignalMessage(), sender);
		break;
	case Content_ClientJoinRequest:
		response = create_client_handler(msg->buffer()->content_as_ClientJoinRequest(), msg->endpoint_UUID());
		break;
	case Content_EntityCreateRequest:
		response = create_entity_handler(msg->buffer()->content_as_EntityCreateRequest(), sender);
		break;
	case Content_FactoryCreateEntityRequest:
		response = factory_create_entity_handler(msg->buffer(), sender);
		break;
	case Content_EntityUpdateRequest:
		response = update_entity_handler(msg->buffer()->content_as_EntityUpdateRequest());
		break;
	case Content_EntityDestroyRequest:
		response = destroy_entity_handler(msg->buffer()->content_as_EntityDestroyRequest());
		break;
	default:
		break;
	}

	if (response != Signal_EMPTY) {
		ZstTransportArgs args;
		args.target_endpoint_UUID = msg->endpoint_UUID();
		args.msg_ID = msg->id();

		router_events()->defer([response, args](std::shared_ptr<ZstStageTransportAdaptor> adaptor) {
			FlatBufferBuilder builder;
			auto signal_offset = CreateSignalMessage(builder, response);
			adaptor->send_msg(Content_SignalMessage, signal_offset.Union(), builder, args);
		});
	}
}

Signal ZstStageHierarchy::signal_handler(const SignalMessage* request, ZstPerformerStageProxy* sender)
{
	if (!sender) {
		return Signal_ERR_STAGE_PERFORMER_NOT_FOUND;
	}

	switch (request->signal()) {
	case Signal_CLIENT_LEAVING:
		return destroy_client(sender);
		break;
	case Signal_CLIENT_HEARTBEAT:
		sender->set_heartbeat_active();
		break;
	case Signal_CLIENT_SYNC:
		//TODO: handled by session
		break;
	}

	return Signal_OK;
}

Signal ZstStageHierarchy::create_client_handler(const ClientJoinRequest * request, uuid endpoint_UUID)
{
	auto client_URI = ZstURI(request->performer()->entity()->URI()->c_str(), request->performer()->entity()->URI()->size());

	//Only one client with this UUID at a time
	if (find_entity(client_URI)) {
		ZstLog::server(LogLevel::warn, "Client already exists ", client_URI.path());
		return Signal_ERR_STAGE_PERFORMER_ALREADY_EXISTS;
	}
	ZstLog::server(LogLevel::notification, "Registering new client {}", client_URI.path());

	// Create proxy
	auto client_proxy = std::make_unique<ZstPerformerStageProxy>(request->performer(), request->graph_reliable_address()->str(), request->graph_unreliable_address()->str(), endpoint_UUID);
	client_proxy->add_adaptor(ZstSynchronisableAdaptor::downcasted_shared_from_this<ZstSynchronisableAdaptor>());

	// Cache new client and its contents
	ZstEntityBundle bundle;
	client_proxy->get_child_entities(bundle, true);
	ZstLog::server(LogLevel::debug, "New performer {} contains:", client_proxy->URI().path());
	for (auto c : bundle) {
		add_entity_to_lookup(c);
		ZstLog::server(LogLevel::debug, " - Entity: {}", c->URI().path());
	}

	// Cache factories
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

	// Update rest of network
	ZstTransportArgs args;
	args.msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;
	auto builder = std::make_shared<FlatBufferBuilder>();
	auto entity_type_vec = std::vector<uint8_t>{ static_cast<uint8_t>(client_proxy->entity_type()) };
	auto entity_vec = std::vector<flatbuffers::Offset<void> >{ client_proxy->serialize(*builder) };
	auto content_message = CreateEntityCreateRequest(*builder, builder->CreateVector(entity_type_vec), builder->CreateVector(entity_vec));
	broadcast(Content_EntityCreateRequest, content_message.Union(), builder, args);
	
	// Move performer
	m_clients[client_proxy->URI()] = std::move(client_proxy);

	return Signal_OK;
}


Signal ZstStageHierarchy::create_entity_handler(const EntityCreateRequest* request, ZstPerformerStageProxy* sender)
{
	// For serialisation later
	auto builder = std::make_shared<FlatBufferBuilder>();
	std::vector< uint8_t> entity_type_vec;
	std::vector< flatbuffers::Offset<void> > entity_vec;

	for (auto it = request->entities_type()->begin(); it != request->entities_type()->end(); ++it)
	{
		auto index = it - request->entities_type()->begin();
		auto entity_type = static_cast<EntityTypes>(*it);
		const EntityData* entity_field = get_entity_field(entity_type, request->entities()->Get(index));
		auto entity_path = ZstURI(entity_field->URI()->c_str(), entity_field->URI()->size());

		ZstLog::server(LogLevel::notification, "Registering new proxy entity {}", entity_path.path());
		if (sender->URI().first() != entity_path.first()) {
			//A performer is requesting this entity be attached to another performer
			ZstLog::server(LogLevel::warn, "TODO: Performer requesting new entity to be attached to another performer", entity_path.path());
			return Signal_ERR_ENTITY_NOT_FOUND;
		}

		ZstHierarchy::add_proxy_entity(entity_type, entity_field, request->entities()->Get(index));
		ZstEntityBase* proxy = find_entity(entity_path);
		if (!proxy) {
			ZstLog::server(LogLevel::warn, "No proxy entity found");
			return Signal_ERR_ENTITY_NOT_FOUND;
		}

		//Dispatch internal module events
		dispatch_entity_arrived_event(proxy);

		// Serialize new entity
		entity_type_vec.push_back(proxy->entity_type());
		entity_vec.push_back(proxy->serialize(*builder));
	}

	//Update rest of network
	if (entity_vec.size()) {
		ZstTransportArgs args;
		args.msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;

		auto entity_msg = CreateEntityCreateRequest(*builder, builder->CreateVector(entity_type_vec), builder->CreateVector(entity_vec));
		broadcast(Content_EntityCreateRequest, entity_msg.Union(), builder, args);
	}

	return Signal_OK;
}

Signal ZstStageHierarchy::factory_create_entity_handler(const StageMessage* request, ZstPerformerStageProxy* sender)
{
	auto create_request = request->content_as_FactoryCreateEntityRequest();
	auto creatable_path = ZstURI(create_request->creatable_entity_URI()->c_str(), create_request->creatable_entity_URI()->size());
	auto factory_path = creatable_path.parent();
	
	ZstLog::server(LogLevel::notification, "Forwarding creatable entity request {} with id {}", creatable_path.path(), request->id());

	ZstEntityFactory* factory = dynamic_cast<ZstEntityFactory*>(find_entity(factory_path));
	if (!factory) {
		ZstLog::server(LogLevel::error, "Could not find factory {}", factory_path.path());
		return Signal_ERR_ENTITY_NOT_FOUND;
	}

	//Find the performer that owns the factory
	ZstPerformerStageProxy* factory_performer = dynamic_cast<ZstPerformerStageProxy*>(find_entity(factory_path.first()));

	//Check to see if one client is already connected to the other
	if (!factory_performer){
		ZstLog::server(LogLevel::error, "Could not find factory {}", factory_path.path());
		return Signal_ERR_STAGE_PERFORMER_NOT_FOUND;
	}

	//Send creatable message to the performer that owns the factory
	ZstMsgID response_id = request->id();
	ZstTransportArgs args;
	args.msg_send_behaviour = ZstTransportRequestBehaviour::ASYNC_REPLY;
	args.on_recv_response = [this, factory_performer, sender, factory_path, response_id](ZstMessageReceipt receipt) {
		if (receipt.status == Signal_ERR_ENTITY_NOT_FOUND) {
			ZstLog::server(LogLevel::error, "Creatable request failed at origin with status {}", EnumNameSignal(receipt.status));
			return;
		}
		ZstLog::server(LogLevel::notification, "Remote factory created entity {}", factory_path.path());
		reply_with_signal(sender, Signal_OK, response_id);
	};

	//Send 
	auto builder = std::make_shared<FlatBufferBuilder>();
	auto create_entity_request = CreateFactoryCreateEntityRequest(*builder, builder->CreateString(create_request->creatable_entity_URI()->str()), builder->CreateString(create_request->name()->str()));
	whisper(factory_performer, Content_FactoryCreateEntityRequest, create_entity_request.Union(), builder, args);

	return Signal_EMPTY;
}

Signal ZstStageHierarchy::update_entity_handler(const EntityUpdateRequest* request)
{
	// For serialisation later
	auto builder = std::make_shared<FlatBufferBuilder>();
	EntityBuilder entity_builder(*builder);
	std::vector< flatbuffers::Offset<void> > entity_vec;
	std::vector< uint8_t > entity_type_vec;

	for (auto it = request->entities_type()->begin(); it != request->entities_type()->end(); ++it){
		auto index = it - request->entities_type()->begin();
		auto entity_type = static_cast<EntityTypes>(*it);
		auto entity_field = get_entity_field(entity_type, request->entities()->Get(index));
		auto entity_path = ZstURI(entity_field->URI()->c_str(), entity_field->URI()->size());

		ZstLog::server(LogLevel::notification, "Updating proxy entity {}", entity_path.path());
		auto proxy = find_entity(entity_path);
		if (proxy) {
			ZstHierarchy::update_proxy_entity(entity_type, entity_field, request->entities()->Get(index));
			entity_type_vec.push_back(static_cast<uint8_t>(entity_type));
			entity_vec.push_back(proxy->serialize(*builder));
		}
	}

	// Update rest of network
	if (entity_vec.size()) {
		ZstTransportArgs args;
		args.msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;
		auto entity_msg = CreateEntityCreateRequest(*builder, builder->CreateVector(entity_type_vec), builder->CreateVector(entity_vec));
		broadcast(Content_EntityUpdateRequest, entity_msg.Union(), builder, args);
	}
	
	return Signal_OK;
}

Signal ZstStageHierarchy::destroy_entity_handler(const EntityDestroyRequest* request)
{
	auto entity_path = ZstURI(request->URI()->c_str(), request->URI()->size());
	auto entity = find_entity(entity_path);
	if (!entity)
		return Signal_ERR_ENTITY_NOT_FOUND;

	ZstLog::server(LogLevel::notification, "Removing proxy entity {}", entity_path.path());

	//Remove the entity
	ZstHierarchy::remove_proxy_entity(entity);

	//Update rest of network first
	ZstTransportArgs args;
	args.msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;
	auto builder = std::make_shared<FlatBufferBuilder>();
	auto destroy_msg_offset = CreateEntityDestroyRequest(*builder, builder->CreateString(request->URI()->str()));
	broadcast(Content_EntityDestroyRequest, destroy_msg_offset.Union(), builder, args);

	return Signal_OK;
}

Signal ZstStageHierarchy::destroy_client(ZstPerformerStageProxy* performer)
{
	//Nothing to do
	if (performer == NULL) {
		return Signal_ERR_STAGE_PERFORMER_NOT_FOUND;
	}

	ZstLog::server(LogLevel::notification, "Performer {} leaving", performer->URI().path());

	//Remove entity and children from lookup
	ZstEntityBundle bundle;
	performer->get_child_entities(bundle, true);
	for (auto c : bundle) {
		remove_entity_from_lookup(c->URI());
	}

	remove_proxy_entity(performer);
	
	//Remove client and call all destructors in its hierarchy
	//auto client_it = m_clients.find(performer->URI());
	//if (client_it != m_clients.end()) {
	//	m_clients.erase(client_it);
	//}

	return Signal_OK;
}

void ZstStageHierarchy::reply_with_signal(ZstPerformerStageProxy* performer, Signal signal, ZstMsgID request_id)
{
	auto builder = std::make_shared<FlatBufferBuilder>();
	ZstTransportArgs args;
	args.msg_ID = request_id;
	auto signal_offset = CreateSignalMessage(*builder, signal);
	whisper(performer, Content_SignalMessage, signal_offset.Union(), builder, args);
}

void ZstStageHierarchy::broadcast(Content message_type, flatbuffers::Offset<void> message_content, std::shared_ptr<flatbuffers::FlatBufferBuilder> & buffer_builder, const ZstTransportArgs& args)
{
	ZstEntityBundle bundle;
	for (auto entity : get_performers(bundle))
	{
		//Can only send messages to performers
		ZstPerformerStageProxy* performer = dynamic_cast<ZstPerformerStageProxy*>(entity);
		if (!performer) {
			ZstLog::server(LogLevel::error, "Not a performer");
			continue;
		}

		//Send message to client
		whisper(performer, message_type, message_content, buffer_builder, args);
	}
}

void ZstStageHierarchy::whisper(ZstPerformerStageProxy* performer, Content message_type, flatbuffers::Offset<void> message_content, std::shared_ptr<flatbuffers::FlatBufferBuilder> buffer_builder, const ZstTransportArgs& args)
{
	ZstTransportArgs endpoint_args = args;
	endpoint_args.target_endpoint_UUID = performer->endpoint_UUID();
	router_events()->defer([this, message_type, message_content, buffer_builder, endpoint_args](std::shared_ptr<ZstStageTransportAdaptor> adaptor) {
		adaptor->send_msg(message_type, message_content, *buffer_builder, endpoint_args);
	});
}

ZstPerformerStageProxy* ZstStageHierarchy::get_client_from_endpoint_UUID(const uuid& endpoint_UUID)
{
	for (auto&& p : m_clients) {
		auto performer_proxy = dynamic_cast<ZstPerformerStageProxy*>(p.second.get());
		if (performer_proxy) {
			if (performer_proxy->endpoint_UUID() == endpoint_UUID)
				return performer_proxy;
		}
	}
	return NULL;
}

}