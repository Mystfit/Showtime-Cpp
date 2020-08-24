#include "ZstClientHierarchy.h"
#include "../core/transports/ZstStageTransport.h"

using namespace flatbuffers;

namespace showtime::client {

ZstClientHierarchy::ZstClientHierarchy() :
	m_root(NULL)
{
}

ZstClientHierarchy::~ZstClientHierarchy()
{
    //Reset local performer
    if(m_root){
        ZstEntityBundle bundle;
        m_root->get_child_entities(&bundle, true, true);
        for (auto entity : bundle) {
            destroy_entity_complete(entity);
        }
    }
}

void ZstClientHierarchy::init(std::string name)
{
	init_adaptors();

	//Create a root entity to hold our local entity hierarchy
    m_root = std::make_shared<ZstPerformer>(name.c_str());

	// Make sure the root performer registers our hierarchy adaptors
	register_entity(m_root.get());
}

void ZstClientHierarchy::process_events()
{
	ZstHierarchy::process_events();
	ZstClientModule::process_events();
}

void ZstClientHierarchy::flush_events()
{
	ZstHierarchy::flush_events();
	ZstClientModule::flush_events();
}

void ZstClientHierarchy::on_receive_msg(const std::shared_ptr<ZstStageMessage>& stage_msg)
{
    switch (stage_msg->type()) {
		case Content_ClientLeaveRequest:
			client_leaving_handler(stage_msg->buffer()->content_as_ClientLeaveRequest());
			break;
        case Content_EntityCreateRequest:
            create_proxy_entity_handler(stage_msg->buffer()->content_as_EntityCreateRequest());
            break;
        case Content_FactoryCreateEntityRequest:
            factory_create_entity_handler(stage_msg->buffer()->content_as_FactoryCreateEntityRequest(), stage_msg->id());
            break;
        case Content_EntityUpdateRequest:
            update_proxy_entity_handler(stage_msg->buffer()->content_as_EntityUpdateRequest());
            break;
        case Content_EntityDestroyRequest:
            destroy_entity_handler(stage_msg->buffer()->content_as_EntityDestroyRequest());
            break;
        default:
            break;
	}
}

void ZstClientHierarchy::publish_entity_update(ZstEntityBase * entity, const ZstURI & original_path)
{
	if (!entity->is_proxy()) {        
		stage_events()->invoke([entity, &original_path](ZstStageTransportAdaptor* adaptor) {
            // Create transport args
			ZstTransportArgs args;
			args.msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;
            
            // Serialize entity into buffer
            auto builder = std::make_shared< FlatBufferBuilder>();
			auto update_offset = CreateEntityUpdateRequest(
				*builder,
				entity->serialized_entity_type(),
				entity->serialize(*builder),
				builder->CreateString(original_path.path(), original_path.full_size())
			);
            
            // Send message
            adaptor->send_msg(Content_EntityUpdateRequest, update_offset.Union(), builder, args);
		});
	}
}

void ZstClientHierarchy::request_entity_activation(ZstEntityBase * entity)
{
	activate_entity(entity, ZstTransportRequestBehaviour::SYNC_REPLY, [](const ZstMessageResponse& r) {});
}

void ZstClientHierarchy::request_entity_registration(ZstEntityBase* entity)
{
	register_entity(entity);
}

void ZstClientHierarchy::activate_entity(ZstEntityBase * entity, const ZstTransportRequestBehaviour & sendtype)
{
	activate_entity(entity, sendtype, [](const ZstMessageResponse& r) {});
}

void ZstClientHierarchy::activate_entity(ZstEntityBase * entity, const ZstTransportRequestBehaviour & sendtype, ZstMessageReceivedAction callback)
{
	if(!entity){
		Log::net(Log::Level::error, "Can't activate a null entity");
		return;
	}

	// Entities need a parent before they can be activated
	if (entity->parent_address().is_empty()) {
        Log::net(Log::Level::warn, "{} has no parent", entity->URI().path());
        return;
	}

	if (entity->is_activated())
		return;
	 
    //Super activation
	ZstHierarchy::activate_entity(entity, sendtype);

	//Send message
	stage_events()->invoke([this, entity, sendtype, callback](ZstStageTransportAdaptor* adaptor){
		//Build message
		ZstTransportArgs args;
		args.msg_send_behaviour = sendtype;
		args.on_recv_response = [this, entity, callback](const ZstMessageResponse& response) {
			if (!ZstStageTransport::verify_signal(response.response, Signal_OK, "Activate entity"))
				return;
			this->activate_entity_complete(entity);
			callback(response);
		};

		ZstEntityBundle bundle;
		entity->get_child_entities(&bundle, true, true);
		for (auto c : bundle) {
			auto builder = std::make_shared< FlatBufferBuilder>();
			auto content_message = CreateEntityCreateRequest(*builder, c->serialized_entity_type(), c->serialize(*builder));
			adaptor->send_msg(Content_EntityCreateRequest, content_message.Union(), builder, args);
		}
	});

	if (sendtype == ZstTransportRequestBehaviour::SYNC_REPLY)
		process_events();
}


void ZstClientHierarchy::deactivate_entity(ZstEntityBase * entity, const ZstTransportRequestBehaviour & sendtype)
{
	if (!entity) return;

	ZstHierarchy::deactivate_entity(entity, sendtype);

	//If the entity is local, let the stage know it's leaving
	if (!entity->is_proxy()) {
		//Send message
		stage_events()->invoke([this, entity, sendtype](ZstStageTransportAdaptor* adaptor) {
			ZstTransportArgs args;
			args.msg_send_behaviour = sendtype;

			if (args.msg_send_behaviour != ZstTransportRequestBehaviour::PUBLISH) {
				args.on_recv_response = [this, entity](ZstMessageResponse response) {
					if (ZstStageTransport::verify_signal(response.response, Signal_OK, "Destroy entity"))
						this->destroy_entity_complete(entity);
				};
			}
			
			auto builder = std::make_shared< FlatBufferBuilder>();
            auto content_message = CreateEntityDestroyRequest(*builder, builder->CreateString(entity->URI().path(), entity->URI().full_size()));
            adaptor->send_msg(Content_EntityDestroyRequest, content_message.Union(), builder, args);
		});
	}
	else {
		destroy_entity_complete(entity);
	}

	// We can immediately clear this entity if we were triggered from a destructor
	if (sendtype == ZstTransportRequestBehaviour::PUBLISH) {
		this->destroy_entity_complete(entity);
	}
	if (sendtype == ZstTransportRequestBehaviour::SYNC_REPLY) {
		process_events();
	}
}

ZstEntityBase * ZstClientHierarchy::create_entity(const ZstURI & creatable_path, const char * name)
{
	return create_entity(creatable_path, name, ZstTransportRequestBehaviour::ASYNC_REPLY);
}

ZstEntityBase * ZstClientHierarchy::create_entity(const ZstURI & creatable_path, const char * name, const ZstTransportRequestBehaviour & sendtype)
{
	ZstEntityBase * entity = NULL;
	//Find the factory associated with this creatable path
	ZstEntityFactory * factory = dynamic_cast<ZstEntityFactory*>(find_entity(creatable_path.parent()));
	if (!factory) {
		Log::net(Log::Level::warn, "Could not find factory to create entity {}", creatable_path.path());
		return NULL;
	}

	ZstURI entity_name(name);

	//Internal factory
	if (!factory->is_proxy()) {
        auto entity = ZstHierarchy::create_entity(creatable_path, name, sendtype);
        get_local_performer()->add_child(entity);
        return entity;
    }
    
    //External factory
    stage_events()->invoke([this, sendtype, creatable_path, &entity, entity_name, factory](ZstStageTransportAdaptor* adaptor) {
		ZstTransportArgs args;
		args.msg_send_behaviour = sendtype;
		args.on_recv_response = [this, &entity, sendtype, creatable_path, entity_name, factory](ZstMessageResponse response) {
			// Convert messages
			auto stage_msg = std::dynamic_pointer_cast<ZstStageMessage>(response.response);
			if (!stage_msg)
				return;

			auto signal = ZstStageTransport::get_signal(response.response);
			if (signal != Signal_EMPTY) {
				Log::net(Log::Level::error, "Entity creation failed with signal {}", EnumNameSignal(signal));
				return;
			}

			auto ack_msg = stage_msg->buffer()->content_as_FactoryCreateEntityACK();
			auto created_entity_path = ZstURI(ack_msg->created_entity_URI()->c_str(), ack_msg->created_entity_URI()->size());
			
			// Find local entity. We would have already received this as a broadcast BEFORE the ack
			auto created_entity = entity = find_entity(created_entity_path);
			if (created_entity) {
				Log::net(Log::Level::notification, "Created entity from {}", created_entity_path.path());
				// Dispatch events
				ZstEntityFactory::detail::dispatch_created_entity_events(factory, created_entity);
				

				if (sendtype == ZstTransportRequestBehaviour::SYNC_REPLY) {
					// Can return the entity since the pointer reference will still be on the stack
					entity = created_entity;
					process_events();
				}
			}
		};
        
		auto builder = std::make_shared< FlatBufferBuilder>();
        auto content_msg = CreateFactoryCreateEntityRequest(*builder, builder->CreateString(creatable_path.path(), creatable_path.full_size()),  builder->CreateString(entity_name.path(), entity_name.full_size()));
        adaptor->send_msg(Content_FactoryCreateEntityRequest, content_msg.Union(), builder, args);
    });

	return entity;
}
    
void ZstClientHierarchy::client_leaving_handler(const ClientLeaveRequest* request)
{
	auto performer_path = ZstURI(request->performer_URI()->c_str(), request->performer_URI()->size());

	if (request->reason() != ClientLeaveReason_QUIT) {
		Log::net(Log::Level::warn, "Performer {} left the graph with reason {}", performer_path.path(), EnumNameClientLeaveReason(request->reason()));
	}

	destroy_entity_complete(find_entity(performer_path));
}

void ZstClientHierarchy::create_proxy_entity_handler(const EntityCreateRequest * request)
{
	add_proxy_entity(create_proxy_entity(request->entity_type(), get_entity_field(request->entity_type(), request->entity()), request->entity()));
}
    
void ZstClientHierarchy::update_proxy_entity_handler(const EntityUpdateRequest * request)
{
	auto original_path = ZstURI(request->original_path()->c_str(), request->original_path()->size());
	auto proxy = find_entity(original_path);
	update_proxy_entity(proxy, request->entity_type(), get_entity_field(request->entity_type(), request->entity()), request->entity());
}
    
void ZstClientHierarchy::destroy_entity_handler(const EntityDestroyRequest * request)
{
    auto entity = find_entity(ZstURI(request->URI()->c_str(), request->URI()->size()));
    destroy_entity_complete(entity);
}


void ZstClientHierarchy::factory_create_entity_handler(const FactoryCreateEntityRequest * request, ZstMsgID request_id)
{
	auto creatable_path = ZstURI(request->creatable_entity_URI()->c_str(), request->creatable_entity_URI()->size());
    auto name = std::string(request->name()->c_str(), request->name()->size());
    
	Log::net(Log::Level::notification, "Received remote request to create a {} entity with the name {} ", creatable_path.path(), name);

	//Find the factory and delegate the entity creation to the main event loop thread
	ZstEntityFactory * factory = dynamic_cast<ZstEntityFactory*>(find_entity(creatable_path.parent()));
	if (!factory) {
		Log::net(Log::Level::warn, "Could not find factory to create entity {}", creatable_path.path());
		return;
	}

	// Create entity
	ZstEntityBase * entity = factory->create_entity(creatable_path, name.c_str());

	if (entity) {
        // Add entity to local perfofrmer
        this->get_local_performer()->add_child(entity, false);
            
        // Activate entity separately
		Log::net(Log::Level::notification, "Activating creatable {} ", entity->URI().path());
		this->activate_entity(entity, ZstTransportRequestBehaviour::ASYNC_REPLY, [this, entity, request_id](ZstMessageResponse response) {
			if (!ZstStageTransport::verify_signal(response.response, Signal_OK, fmt::format("Creatable {} activation request timed out", entity->URI().path())))
				return;

			Log::net(Log::Level::notification, "Creatable {} activated", entity->URI().path());
			stage_events()->invoke([request_id](ZstStageTransportAdaptor* adaptor) {
				ZstTransportArgs args;
                args.msg_ID = request_id;
                    
                // Send signal
				auto builder = std::make_shared< FlatBufferBuilder>();
                auto signal = CreateSignalMessage(*builder, Signal_OK);
                adaptor->send_msg(Content_SignalMessage, signal.Union(), builder, args);
			});
		});
	}
	else {
		stage_events()->invoke([request_id](ZstStageTransportAdaptor* adaptor) {
			ZstTransportArgs args;
            args.msg_ID = request_id;
                
            // Send signals
			auto builder = std::make_shared< FlatBufferBuilder>();
            auto signal = CreateSignalMessage(*builder, Signal_ERR_ENTITY_NOT_FOUND);
            adaptor->send_msg(Content_SignalMessage, signal.Union(), builder, args);
		});
	}
}

void ZstClientHierarchy::activate_entity_complete(ZstEntityBase * entity)
{
	ZstHierarchy::activate_entity_complete(entity);

	ZstEntityBundle bundle;
    entity->get_child_entities(&bundle, true, true);
	for (auto c : bundle) {
		hierarchy_events()->invoke([c](ZstHierarchyAdaptor* adaptor) { 
			adaptor->on_entity_arriving(c); 
		});
	}
}

void ZstClientHierarchy::destroy_entity_complete(ZstEntityBase * entity)
{
	if (!entity) {
		//Log::net(Log::Level::warn, "destroy_entity_complete(): Entity not found");
		return;
	}

	if (entity->URI() == this->get_local_performer()->URI()) {
		//Log::net(Log::Level::debug, "Destroyed entity is our own client, ignore.");
		return;
	}
	ZstHierarchy::destroy_entity_complete(entity);
}

void ZstClientHierarchy::update_entity_URI(ZstEntityBase* entity, const ZstURI& original_path)
{
	ZstHierarchy::update_entity_URI(entity, original_path);
}

ZstEntityBase * ZstClientHierarchy::find_entity(const ZstURI & path) const
{
	if (m_root->URI() == path) {
		return m_root.get();
	}
	return ZstHierarchy::find_entity(path);
}

bool ZstClientHierarchy::path_is_local(const ZstURI & path) 
{
	return path.contains(m_root->URI());
}

std::unique_ptr<ZstEntityBase> ZstClientHierarchy::create_proxy_entity(const EntityTypes entity_type, const EntityData* entity_data, const void* payload) 
{
	// Don't need to activate local entities, they will auto-activate when the stage responds with an OK
	// Also, we can't rely on the proxy flag here as it won't have been set yet
    auto entity_path = ZstURI(entity_data->URI()->c_str(), entity_data->URI()->size());
	auto local_path = get_local_performer()->URI();

	Log::net(Log::Level::debug, "{}: Received proxy entity {}", get_local_performer()->URI().path(), entity_path.path());

	if (entity_path.contains(get_local_performer()->URI())) {
		Log::net(Log::Level::debug, "Proxy entity {} is local . Ignoring", entity_path.path());
		return NULL;
    }
	return ZstHierarchy::create_proxy_entity(entity_type, entity_data, payload);
}

void ZstClientHierarchy::update_proxy_entity(ZstEntityBase* original, const EntityTypes entity_type, const EntityData* entity_data, const void* payload)
{
    auto entity_path = ZstURI(entity_data->URI()->c_str(), entity_data->URI()->size());
    
	//Don't need to update local entities, they should have published the update
	if (path_is_local(entity_path)) {
		Log::net(Log::Level::debug, "Don't need to update a local entity {}. Ignoring", entity_path.path());
		return;
	}
    
	ZstHierarchy::update_proxy_entity(original, entity_type, entity_data, payload);
}

ZstPerformer * ZstClientHierarchy::get_local_performer() const
{
	return m_root.get();
}

}
