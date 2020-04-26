#include "ZstStageHierarchy.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/nil_generator.hpp>
#include "../core/transports/ZstStageTransport.h"

using namespace boost::uuids;
using namespace flatbuffers;

namespace showtime {

ZstStageHierarchy::~ZstStageHierarchy()
{
}

void ZstStageHierarchy::init_adaptors()
{
	ZstHierarchy::init_adaptors();
}

void ZstStageHierarchy::set_wake_condition(std::shared_ptr<ZstSemaphore>& condition)
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

void ZstStageHierarchy::on_entity_arriving(ZstEntityBase* entity)
{
	//Don't send the proxy entity back to its origin client
	auto excluded = std::vector<ZstPerformer*>{ dynamic_cast<ZstPerformerStageProxy*>(find_entity(entity->URI().first())) };

	// Update rest of network
	ZstTransportArgs args;
	args.msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;
	auto builder = std::make_shared<FlatBufferBuilder>();
	auto content_message = CreateEntityCreateRequest(*builder, entity->serialized_entity_type(), entity->serialize(*builder));
	ZstLog::server(LogLevel::debug, "Broadcasting entity {}", entity->URI().path());
	broadcast(Content_EntityCreateRequest, content_message.Union(), builder, args, excluded);
}

void ZstStageHierarchy::on_factory_arriving(ZstEntityFactory* factory)
{
	on_entity_arriving(factory);
}

void ZstStageHierarchy::on_performer_arriving(ZstPerformer* performer)
{
	on_entity_arriving(performer);
}

void ZstStageHierarchy::on_receive_msg(const std::shared_ptr<ZstStageMessage>& msg)
{
	Signal response = Signal_EMPTY;
	ZstPerformerStageProxy* sender = get_client_from_endpoint_UUID(msg->origin_endpoint_UUID());
	if (msg->type() != Content_ClientJoinRequest && !sender) {
		ZstLog::server(LogLevel::warn, "Received {} message but the sender could not be found", EnumNameContent(msg->type()));
	}

	switch (msg->type()) {
	case Content_SignalMessage:
		response = signal_handler(msg, sender);
		break;
	case Content_ClientJoinRequest:
		response = create_client_handler(msg);
		break;
	case Content_ClientLeaveRequest:
		response = client_leaving_handler(msg, sender);
		break;
	case Content_EntityCreateRequest:
		response = create_entity_handler(msg, sender);
		break;
	case Content_FactoryCreateEntityRequest:
		response = factory_create_entity_handler(msg, sender);
		break;
	case Content_EntityUpdateRequest:
		response = update_entity_handler(msg, sender);
		break;
	case Content_EntityDestroyRequest:
		response = destroy_entity_handler(msg, sender);
		break;
	default:
		break;
	}

	if (response != Signal_EMPTY) {
		ZstTransportArgs args;
		args.target_endpoint_UUID = msg->origin_endpoint_UUID();
		args.msg_ID = msg->id();
		auto builder = std::make_shared< FlatBufferBuilder>();
		auto signal_offset = CreateSignalMessage(*builder, response);
		if(auto transport = std::dynamic_pointer_cast<ZstStageTransport>(msg->owning_transport()))
			transport->send_msg(Content_SignalMessage, signal_offset.Union(), builder, args);
	}
}

Signal ZstStageHierarchy::signal_handler(const std::shared_ptr<ZstStageMessage>& request, ZstPerformerStageProxy* sender)
{
	if (!sender) {
		return Signal_ERR_STAGE_PERFORMER_NOT_FOUND;
	}

	//ZstLog::server(LogLevel::debug, "Server received heartbeat from {} {}", sender->URI().path(), boost::uuids::to_string(request->id()));

	if (ZstStageTransport::get_signal(request) == Signal_CLIENT_HEARTBEAT) {
		sender->set_heartbeat_active();
		return Signal_OK;
	}

	return Signal_EMPTY;
}

Signal ZstStageHierarchy::create_client_handler(const std::shared_ptr<ZstStageMessage>& request)
{
	auto content = request->buffer()->content_as_ClientJoinRequest();
	auto client_URI = ZstURI(content->performer()->entity()->URI()->c_str(), content->performer()->entity()->URI()->size());

	// Only one client with this UUID at a time
	if (find_entity(client_URI)) {
		ZstLog::server(LogLevel::warn, "Client already exists ", client_URI.path());
		return Signal_ERR_STAGE_PERFORMER_ALREADY_EXISTS;
	}
	ZstLog::server(LogLevel::notification, "Registering new client {}", client_URI.path());

	// Create proxy
	if (auto transport = std::dynamic_pointer_cast<ZstStageTransport>(request->owning_transport())) {
		ZstHierarchy::add_proxy_entity(std::make_unique<ZstPerformerStageProxy>(
			content->performer(),
			(content->graph_reliable_address()) ? content->graph_reliable_address()->str() : "",
			(content->graph_unreliable_address()) ? content->graph_unreliable_address()->str() : "",
			request->origin_endpoint_UUID(),
			std::static_pointer_cast<ZstStageTransport>(transport)
		));
	}

	return Signal_OK;
}

Signal ZstStageHierarchy::client_leaving_handler(const std::shared_ptr<ZstStageMessage>& request, ZstPerformerStageProxy* sender)
{
	auto content = request->buffer()->content_as_ClientLeaveRequest();
	if (content->reason() == ClientLeaveReason_QUIT) {
		ZstLog::server(LogLevel::notification, "Performer {} leaving", sender->URI().path());
	}
	else {
		ZstLog::server(LogLevel::warn, "Performer {} left with reason {}", sender->URI().path(), EnumNameClientLeaveReason(content->reason()));
	}
	remove_proxy_entity(sender);

	//Update rest of network
	auto excluded = std::vector<ZstPerformer*>{ sender };

	ZstTransportArgs args;
	args.msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;
	auto builder = std::make_shared<FlatBufferBuilder>();
	auto destroy_msg_offset = CreateClientLeaveRequest(*builder, builder->CreateString(sender->URI().path()), content->reason());
	broadcast(Content_ClientLeaveRequest, destroy_msg_offset.Union(), builder, args);

	return Signal_OK;
}


Signal ZstStageHierarchy::create_entity_handler(const std::shared_ptr<ZstStageMessage>& request, ZstPerformerStageProxy* sender)
{
	auto content = request->buffer()->content_as_EntityCreateRequest();
	const EntityData* entity_field = get_entity_field(content->entity_type(), content->entity());
	auto entity_path = ZstURI(entity_field->URI()->c_str(), entity_field->URI()->size());

	ZstLog::server(LogLevel::notification, "Activating new proxy entity {}", entity_path.path());
	if (sender->URI().first() != entity_path.first()) {
		//A performer is requesting this entity be attached to another performer
		ZstLog::server(LogLevel::warn, "TODO: Performer requesting new entity to be attached to another performer", entity_path.path());
		return Signal_ERR_ENTITY_NOT_FOUND;
	}

	ZstHierarchy::add_proxy_entity(create_proxy_entity(content->entity_type(), entity_field, content->entity()));
	ZstEntityBase* proxy = find_entity(entity_path);
	if (!proxy) {
		ZstLog::server(LogLevel::warn, "No proxy entity found");
		return Signal_ERR_ENTITY_NOT_FOUND;
	}
	
	return Signal_OK;
}

Signal ZstStageHierarchy::factory_create_entity_handler(const std::shared_ptr<ZstStageMessage>& request, ZstPerformerStageProxy* sender)
{
	auto content = request->buffer()->content_as_FactoryCreateEntityRequest();
	auto creatable_path = ZstURI(content->creatable_entity_URI()->c_str(), content->creatable_entity_URI()->size());
	auto factory_path = creatable_path.parent();
	
	ZstLog::server(LogLevel::notification, "Forwarding creatable entity request {}", creatable_path.path());

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
    ZstMsgID response_id;
    memcpy(&response_id, request->buffer()->id()->data(), request->buffer()->id()->size());
    
    ZstTransportArgs args;
	args.msg_send_behaviour = ZstTransportRequestBehaviour::ASYNC_REPLY;
    args.on_recv_response = [this, sender, factory_path, response_id](ZstMessageResponse response) {
		if (!ZstStageTransport::verify_signal(response.response, Signal_OK, "Creatable request at origin"))
			return;
		
		ZstLog::server(LogLevel::notification, "Remote factory created entity {}", factory_path.path());
		reply_with_signal(sender, Signal_OK, response_id);
	};

	//Send 
	auto builder = std::make_shared<FlatBufferBuilder>();
	auto create_entity_request = CreateFactoryCreateEntityRequest(*builder, builder->CreateString(content->creatable_entity_URI()->str()), builder->CreateString(content->name()->str()));
	whisper(factory_performer, Content_FactoryCreateEntityRequest, create_entity_request.Union(), builder, args);

	return Signal_EMPTY;
}

Signal ZstStageHierarchy::update_entity_handler(const std::shared_ptr<ZstStageMessage>& request, ZstPerformerStageProxy* sender)
{
	auto content = request->buffer()->content_as_EntityUpdateRequest();
	// For serialisation later
	auto builder = std::make_shared<FlatBufferBuilder>();
	auto entity_field = get_entity_field(content->entity_type(), content->entity());
	auto entity_path = ZstURI(entity_field->URI()->c_str(), entity_field->URI()->size());
	auto original_path = ZstURI(content->original_path()->c_str(), content->original_path()->size());

	ZstLog::server(LogLevel::notification, "Updating proxy entity {}", entity_path.path());
	auto proxy = find_entity(original_path);
	
	if (proxy) {
		ZstHierarchy::update_proxy_entity(proxy, content->entity_type(), entity_field, content->entity());

		auto excluded = std::vector<ZstPerformer*>{ sender };
		ZstTransportArgs args;
		args.msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;
		auto entity_msg = CreateEntityUpdateRequest(
			*builder, 
			content->entity_type(),
			proxy->serialize(*builder), 
			builder->CreateString(content->original_path()->c_str(), content->original_path()->size())
		);
		broadcast(Content_EntityUpdateRequest, entity_msg.Union(), builder, args, excluded);
	}
	
	return Signal_OK;
}

Signal ZstStageHierarchy::destroy_entity_handler(const std::shared_ptr<ZstStageMessage>& request, ZstPerformerStageProxy* sender)
{
	auto content = request->buffer()->content_as_EntityDestroyRequest();
	auto entity_path = ZstURI(content->URI()->c_str(), content->URI()->size());
	auto entity = find_entity(entity_path);
	if (!entity)
		return Signal_ERR_ENTITY_NOT_FOUND;

	ZstLog::server(LogLevel::notification, "Removing proxy entity {}", entity_path.path());

	//Remove the entity
	ZstHierarchy::remove_proxy_entity(entity);

	//Update rest of network first
	auto excluded = std::vector<ZstPerformer*>{ sender };
	ZstTransportArgs args;
	args.msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;
	auto builder = std::make_shared<FlatBufferBuilder>();
	auto destroy_msg_offset = CreateEntityDestroyRequest(*builder, builder->CreateString(content->URI()->str()));
	broadcast(Content_EntityDestroyRequest, destroy_msg_offset.Union(), builder, args);

	return Signal_OK;
}

void ZstStageHierarchy::request_entity_registration(ZstEntityBase* entity)
{
	register_entity(entity);
}

void ZstStageHierarchy::reply_with_signal(ZstPerformerStageProxy* performer, Signal signal, ZstMsgID request_id)
{
	auto builder = std::make_shared<FlatBufferBuilder>();
	ZstTransportArgs args;
	args.msg_ID = request_id;
	auto signal_offset = CreateSignalMessage(*builder, signal);
	whisper(performer, Content_SignalMessage, signal_offset.Union(), builder, args);
}

void ZstStageHierarchy::broadcast(Content message_type, flatbuffers::Offset<void> message_content, std::shared_ptr<flatbuffers::FlatBufferBuilder> & buffer_builder, const ZstTransportArgs& args, const std::vector<ZstPerformer*> & excluded)
{
	ZstLog::server(LogLevel::debug, "Broadcasting {} message {}", EnumNameContent(message_type), boost::uuids::to_string(args.msg_ID));
	ZstEntityBundle bundle;
	for (auto entity : get_performers(bundle))
	{
		//Can only send messages to performers
		ZstPerformerStageProxy* performer = dynamic_cast<ZstPerformerStageProxy*>(entity);
		if (!performer || std::find(excluded.begin(), excluded.end(), performer) != excluded.end()) {
			continue;
		}

		//Send message to client
		whisper(performer, message_type, message_content, buffer_builder, args);
	}
}

void ZstStageHierarchy::whisper(ZstPerformerStageProxy* performer, Content message_type, flatbuffers::Offset<void> message_content, std::shared_ptr<flatbuffers::FlatBufferBuilder>& buffer_builder, const ZstTransportArgs& args)
{
	ZstTransportArgs endpoint_args = args;
	endpoint_args.target_endpoint_UUID = performer->origin_endpoint_UUID();
	ZstLog::server(LogLevel::debug, "Whispering {} message {} to {}", EnumNameContent(message_type), boost::uuids::to_string(args.msg_ID), boost::uuids::to_string(performer->origin_endpoint_UUID()));

	if (auto transport = performer->origin_transport().lock())
		transport->send_msg(message_type, message_content, buffer_builder, endpoint_args);

	//router_events()->defer([this, message_type, message_content, buffer_builder, endpoint_args](std::shared_ptr<ZstStageTransportAdaptor> adaptor) mutable {
	//	adaptor->send_msg(message_type, message_content, buffer_builder, endpoint_args);
	//});
}

ZstPerformerStageProxy* ZstStageHierarchy::get_client_from_endpoint_UUID(const uuid& origin_endpoint_UUID)
{
	ZstEntityBundle clients;
	get_performers(clients);
	for (auto p : clients) {
		auto performer_proxy = dynamic_cast<ZstPerformerStageProxy*>(p);
		if (performer_proxy) {
			if (performer_proxy->origin_endpoint_UUID() == origin_endpoint_UUID)
				return performer_proxy;
		}
	}
	//auto client_it = m_clients.find(origin_endpoint_UUID);
	//if (client_it != m_clients.end())
	//	return client_it->second;
	return NULL;
}

//ZstPerformerStageProxy* ZstStageHierarchy::get_endpoint(const uuid& origin_endpoint_UUID) {
//	
//}


}
