#include "ZstClientHierarchy.h"
#include "../core/transports/ZstStageTransport.h"
#include <showtime/ZstFormat.h>
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
		case Content_OfflineEntitiesNotification:
			offline_entities_notification_handler(stage_msg->buffer()->content_as_OfflineEntitiesNotification());
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
			FlatBufferBuilder builder;
			auto update_offset = CreateEntityUpdateRequest(
				builder,
				entity->serialized_entity_type(),
				entity->serialize(builder),
				builder.CreateString(original_path.path(), original_path.full_size())
			);
            
            // Send message
            adaptor->send_msg(adaptor->create_msg(Content_EntityUpdateRequest, update_offset.Union(), builder), args);
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

void ZstClientHierarchy::activate_entity(ZstEntityBase* entity, const ZstTransportRequestBehaviour& sendtype, ZstMessageReceivedAction callback)
{
	ZstEntityBundle bundle;
	entity->get_child_entities(&bundle, true, true);
	activate_entity_batched(bundle, sendtype, callback);
}

void ZstClientHierarchy::activate_entity_batched(ZstBundle<ZstEntityBase*> entities, const ZstTransportRequestBehaviour& sendtype)
{
	activate_entity_batched(entities, sendtype, [](const ZstMessageResponse& r) {});
}

void ZstClientHierarchy::activate_entity_batched(ZstBundle<ZstEntityBase*> entities, const ZstTransportRequestBehaviour& sendtype, ZstMessageReceivedAction callback)
{
	if (!entities.size()) {
		Log::net(Log::Level::error, "Can't activate a null entity");
		return;
	}

	//Super activation
	ZstHierarchy::activate_entity_batched(entities, sendtype);

	//Send message
	stage_events()->invoke([this, entities, sendtype, callback](ZstStageTransportAdaptor* adaptor) {
		//Build message
		ZstTransportArgs args;
		args.msg_send_behaviour = sendtype;
		args.on_recv_response = [this, entities, callback](const ZstMessageResponse& response) {
			if (!ZstStageTransport::verify_signal(response.response, Signal_OK, "Activate entity"))
				return;

			for (ZstEntityBase* entity : entities) {
				this->activate_entity_complete(entity);
			}
			callback(response);
		};

		// Set up flatbuffer builder and temporary buffers
		FlatBufferBuilder builder;
		std::vector<uint8_t> entity_types;
		std::vector<flatbuffers::Offset<void>> entities_serialized;
		entity_types.resize(entities.size());
		entities_serialized.resize(entities.size());

		// Split bundle into entity types and serialized entities
		for(auto i = 0; i < entities.size(); i++){
			auto entity = entities[i];
			entity_types[i] = static_cast<uint8_t>(entity->serialized_entity_type());
			entities_serialized[i] = entity->serialize(builder);
		}

		// Convert vectors to flatbuffer offsets
		flatbuffers::Offset<flatbuffers::Vector<uint8_t>> entityTypesSerialized = builder.CreateVector(entity_types);
		auto entitiesSerializedFB = builder.CreateVector(entities_serialized);

		auto content_message = CreateEntityCreateRequest(builder, entityTypesSerialized, entitiesSerializedFB);
		adaptor->send_msg(adaptor->create_msg(Content_EntityCreateRequest, content_message.Union(), builder), args);
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
			
			FlatBufferBuilder builder;
            auto content_message = CreateEntityDestroyRequest(builder, builder.CreateString(entity->URI().path(), entity->URI().full_size()));
            adaptor->send_msg(adaptor->create_msg(Content_EntityDestroyRequest, content_message.Union(), builder), args);
		});
	}
	else {
		// Entity is a proxy - immediately destroy locally
		destroy_entity_complete(entity);
	}

	// For local entities triggered from a destructor (PUBLISH), immediately clear
	// Note: Proxy entities are already handled above, so only call for non-proxy entities
	if (!entity->is_proxy() && sendtype == ZstTransportRequestBehaviour::PUBLISH) {
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
		if(entity)
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
        
		FlatBufferBuilder builder;
        auto content_msg = CreateFactoryCreateEntityRequest(builder, builder.CreateString(creatable_path.path(), creatable_path.full_size()),  builder.CreateString(entity_name.path(), entity_name.full_size()));
        adaptor->send_msg(adaptor->create_msg(Content_FactoryCreateEntityRequest, content_msg.Union(), builder), args);
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
	for (uoffset_t i = 0; i < request->entity()->size(); ++i) {
		EntityTypes entity_type = static_cast<EntityTypes>(request->entity_type()->Get(i));
		const void* entity_raw = request->entity()->Get(i);

		std::unique_ptr<ZstEntityBase> entity = create_proxy_entity(entity_type, get_entity_field(entity_type, entity_raw), entity_raw);
		ZstEntityBase* entity_ptr = entity.get();
		add_proxy_entity(std::move(entity));
		dispatch_entity_arrived_event(entity_ptr);
	}
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
			if (!ZstStageTransport::verify_signal(response.response, Signal_OK, ZSTformat("Creatable {} activation request timed out", entity->URI().path())))
				return;

			Log::net(Log::Level::notification, "Creatable {} activated", entity->URI().path());
			stage_events()->invoke([request_id](ZstStageTransportAdaptor* adaptor) {
				ZstTransportArgs args;
                args.msg_ID = request_id;
                    
                // Send signal
				FlatBufferBuilder builder;
                auto signal = CreateSignalMessage(builder, Signal_OK);
                adaptor->send_msg(adaptor->create_msg(Content_SignalMessage, signal.Union(), builder), args);
			});
		});
	}
	else {
		stage_events()->invoke([request_id](ZstStageTransportAdaptor* adaptor) {
			ZstTransportArgs args;
            args.msg_ID = request_id;
                
            // Send signals
			FlatBufferBuilder builder;
            auto signal = CreateSignalMessage(builder, Signal_ERR_ENTITY_NOT_FOUND);
            adaptor->send_msg(adaptor->create_msg(Content_SignalMessage, signal.Union(), builder), args);
		});
	}
}

void ZstClientHierarchy::activate_entity_complete(ZstEntityBase * entity)
{
	ZstHierarchy::activate_entity_complete(entity);

	ZstEntityBundle bundle;
    entity->get_child_entities(&bundle, true, true);
	hierarchy_events()->invoke([entity](ZstHierarchyAdaptor* adaptor) {
		adaptor->on_entity_arriving(entity);
	});
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

void ZstClientHierarchy::offline_entities_notification_handler(const OfflineEntitiesNotification* notification)
{
	auto performer_uri = ZstURI(notification->performer_URI()->c_str(), notification->performer_URI()->size());

	Log::net(Log::Level::notification, "Received offline entities notification for performer {}", performer_uri.path());

	// Create proxy entities from the notification
	ZstEntityBundle bundle;
	for (uoffset_t i = 0; i < notification->offline_entities()->size(); ++i) {
		EntityTypes entity_type = static_cast<EntityTypes>(notification->offline_entity_types()->Get(i));
		const void* entity_raw = notification->offline_entities()->Get(i);

		std::unique_ptr<ZstEntityBase> entity = create_proxy_entity(entity_type, get_entity_field(entity_type, entity_raw), entity_raw);
		if (entity) {
			ZstEntityBase* entity_ptr = entity.get();

			// Mark entity as OFFLINE
			synchronisable_set_activation_status(entity_ptr, ZstSyncStatus::OFFLINE);

			// Store in offline entities map
			m_offline_entities[entity_ptr->URI()] = entity_ptr;

			// Add to proxy storage
			add_proxy_entity(std::move(entity));

			// Add to bundle for event
			bundle.add(entity_ptr);

			Log::net(Log::Level::debug, "Stored offline entity {} for reclamation", entity_ptr->URI().path());
		}
	}

	// Fire offline_entities_available event with the bundle
	if (bundle.size() > 0) {
		hierarchy_events()->invoke([performer_uri, &bundle](ZstHierarchyAdaptor* adaptor) {
			adaptor->on_offline_entities_available(performer_uri, &bundle);
		});
	}
}

void ZstClientHierarchy::reclaim_entity(ZstEntityBase* local_entity, const ZstURI& offline_uri)
{
	if (!local_entity) {
		Log::net(Log::Level::error, "Cannot reclaim with null local entity");
		return;
	}

	// Check if the offline entity exists
	auto it = m_offline_entities.find(offline_uri);
	if (it == m_offline_entities.end()) {
		Log::net(Log::Level::warn, "No offline entity found at URI {}", offline_uri.path());
		return;
	}

	Log::net(Log::Level::notification, "Reclaiming offline entity {} with local entity {}", offline_uri.path(), local_entity->URI().path());

	// Send reclaim request to server
	stage_events()->invoke([this, offline_uri, local_entity](ZstStageTransportAdaptor* adaptor) {
		ZstTransportArgs args;
		args.msg_send_behaviour = ZstTransportRequestBehaviour::ASYNC_REPLY;
		args.on_recv_response = [this, offline_uri, local_entity](const ZstMessageResponse& response) {
			if (!ZstStageTransport::verify_signal(response.response, Signal_OK, "Reclaim entity")) {
				Log::net(Log::Level::error, "Failed to reclaim entity {}", offline_uri.path());
				return;
			}

			Log::net(Log::Level::notification, "Successfully reclaimed entity {}", offline_uri.path());

			// Remove from offline entities map
			m_offline_entities.erase(offline_uri);

			// Fire entity_online event
			hierarchy_events()->invoke([local_entity](ZstHierarchyAdaptor* adaptor) {
				adaptor->on_entity_online(local_entity);
			});
		};

		// Build message with entity URIs
		FlatBufferBuilder builder;
		std::vector<flatbuffers::Offset<flatbuffers::String>> uri_offsets;
		uri_offsets.push_back(builder.CreateString(offline_uri.path(), offline_uri.full_size()));

		auto content_message = CreateEntityReclaimRequest(builder, builder.CreateVector(uri_offsets));
		adaptor->send_msg(adaptor->create_msg(Content_EntityReclaimRequest, content_message.Union(), builder), args);
	});
}

void ZstClientHierarchy::reclaim_all_offline_entities()
{
	if (m_offline_entities.empty()) {
		Log::net(Log::Level::debug, "No offline entities to reclaim");
		return;
	}

	Log::net(Log::Level::notification, "Reclaiming {} offline entities", m_offline_entities.size());

	// Build list of all offline entity URIs
	std::vector<ZstURI> offline_uris;
	for (const auto& pair : m_offline_entities) {
		offline_uris.push_back(pair.first);
	}

	// Send reclaim request to server
	stage_events()->invoke([this, offline_uris](ZstStageTransportAdaptor* adaptor) {
		ZstTransportArgs args;
		args.msg_send_behaviour = ZstTransportRequestBehaviour::ASYNC_REPLY;
		args.on_recv_response = [this, offline_uris](const ZstMessageResponse& response) {
			if (!ZstStageTransport::verify_signal(response.response, Signal_OK, "Reclaim all entities")) {
				Log::net(Log::Level::error, "Failed to reclaim all offline entities");
				return;
			}

			Log::net(Log::Level::notification, "Successfully reclaimed all offline entities");

			// Fire entity_online events and clear offline map
			for (const auto& uri : offline_uris) {
				auto entity = find_entity(uri);
				if (entity) {
					hierarchy_events()->invoke([entity](ZstHierarchyAdaptor* adaptor) {
						adaptor->on_entity_online(entity);
					});
				}
			}
			m_offline_entities.clear();
		};

		// Build message with all entity URIs
		FlatBufferBuilder builder;
		std::vector<flatbuffers::Offset<flatbuffers::String>> uri_offsets;
		for (const auto& uri : offline_uris) {
			uri_offsets.push_back(builder.CreateString(uri.path(), uri.full_size()));
		}

		auto content_message = CreateEntityReclaimRequest(builder, builder.CreateVector(uri_offsets));
		adaptor->send_msg(adaptor->create_msg(Content_EntityReclaimRequest, content_message.Union(), builder), args);
	});
}

void ZstClientHierarchy::get_offline_entities(ZstEntityBundle& bundle) const
{
	for (const auto& pair : m_offline_entities) {
		bundle.add(pair.second);
	}
}

}
