#include "ZstStageHierarchy.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/nil_generator.hpp>
#include "../core/transports/ZstStageTransport.h"
#include <showtime/ZstLogging.h>
#include <showtime/entities/ZstEntityBase.h>
#include <showtime/entities/ZstPerformer.h>
#include <showtime/entities/ZstEntityFactory.h>
#include <showtime/schemas/messaging/graph_types_generated.h>
#include <showtime/schemas/messaging/session_generated.h>
#include <algorithm>
#include <set>

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

void ZstStageHierarchy::set_wake_condition(std::shared_ptr<std::condition_variable>& condition)
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
    FlatBufferBuilder builder;
    
    // Convert single entity to a batched EntityCreateRequest
    std::vector<uint8_t> entity_types;
    std::vector<flatbuffers::Offset<void>> entities_serialized;
    entity_types.push_back(static_cast<uint8_t>(entity->serialized_entity_type()));
    entities_serialized.push_back(entity->serialize(builder));
    
    // Convert vectors to flatbuffer offsets
    flatbuffers::Offset<flatbuffers::Vector<uint8_t>> entityTypesSerialized = builder.CreateVector(entity_types);
    auto entitiesSerializedFB = builder.CreateVector(entities_serialized);

    auto content_message = CreateEntityCreateRequest(builder, entityTypesSerialized, entitiesSerializedFB);
    Log::server(Log::Level::debug, "Broadcasting entity {}", entity->URI().path());

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

void ZstStageHierarchy::client_leaving(ZstPerformer* performer, const ClientLeaveReason& reason)
{
    if (!performer) {
        return;
    }

    if (reason == ClientLeaveReason_QUIT) {
        Log::server(Log::Level::notification, "Performer {} leaving", performer->URI().path());
    }
    else {
        Log::server(Log::Level::warn, "Performer {} left with reason {}", performer->URI().path(), EnumNameClientLeaveReason(reason));
    }

    // Check if we should preserve entities as offline
    if (m_preserve_entities_on_disconnect && reason != ClientLeaveReason_PRESERVE_OFFLINE) {
        // Mark entities as offline instead of removing them
        mark_performer_offline(performer);
        return;
    }

    remove_proxy_entity(performer);

    //Update rest of network
    auto excluded = std::vector<ZstPerformer*>{ performer };

    ZstTransportArgs args;
    args.msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;
    FlatBufferBuilder builder;
    auto destroy_msg_offset = CreateClientLeaveRequest(builder, builder.CreateString(performer->URI().path()), reason);
    broadcast(Content_ClientLeaveRequest, destroy_msg_offset.Union(), builder, args, excluded);
}

void ZstStageHierarchy::on_receive_msg(const std::shared_ptr<ZstStageMessage>& msg)
{
    Signal response = Signal_EMPTY;
    ZstPerformerStageProxy* sender = get_client_from_endpoint_UUID(msg->origin_endpoint_UUID());
    if (msg->type() != Content_ClientJoinRequest && !sender) {
        Log::server(Log::Level::warn, "Received {} message but the sender could not be found", EnumNameContent(msg->type()));
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
    case Content_EntityReclaimRequest:
        response = entity_reclaim_handler(msg, sender);
        break;
    default:
        break;
    }

    if (response != Signal_EMPTY) {
        ZstTransportArgs args;
        args.target_endpoint_UUID = msg->origin_endpoint_UUID();
        args.msg_ID = msg->id();
        
        FlatBufferBuilder builder;
        auto signal_offset = CreateSignalMessage(builder, response);
        if(auto transport = std::dynamic_pointer_cast<ZstStageTransport>(msg->owning_transport()))
            transport->send_msg(transport->create_msg(Content_SignalMessage, signal_offset.Union(), builder), args);
    }
}

// ZstSerialisable implementation
void ZstStageHierarchy::serialize_partial(flatbuffers::Offset<void>& destination_offset, flatbuffers::FlatBufferBuilder& builder) const {
    throw std::runtime_error("Partial serialization not supported for Hierarchy");
}

flatbuffers::uoffset_t ZstStageHierarchy::serialize(flatbuffers::FlatBufferBuilder& builder) const {
    // Create flat hierarchy
    ZstEntityBundle entity_bundle;
    ZstEntityBundle performer_bundle;
    get_performers(performer_bundle);
    for (auto performer : performer_bundle) {
        performer->get_child_entities(&entity_bundle, true, true);
    }

    // Build persisted entities with creation source tracking
    // NOTE: Do not persist performers themselves, only their children
    std::vector<flatbuffers::Offset<PersistedEntity>> persisted_entities;

    for (auto entity : entity_bundle) {
        // Skip performers - they will be recreated when clients reconnect
        if (entity->entity_type() == ZstEntityType::PERFORMER) {
            continue;
        }
        // Serialize entity data into a separate builder
        FlatBufferBuilder entity_builder;
        auto entity_offset = entity->serialize(entity_builder);

        // Finish the entity buffer
        // Cast the uoffset_t to the appropriate typed Offset and finish
        switch (entity->serialized_entity_type()) {
            case EntityTypes_Component:
                entity_builder.Finish(flatbuffers::Offset<Component>(entity_offset));
                break;
            case EntityTypes_Performer:
                entity_builder.Finish(flatbuffers::Offset<Performer>(entity_offset));
                break;
            case EntityTypes_Plug:
                entity_builder.Finish(flatbuffers::Offset<Plug>(entity_offset));
                break;
            case EntityTypes_Factory:
                entity_builder.Finish(flatbuffers::Offset<Factory>(entity_offset));
                break;
            default:
                Log::server(Log::Level::error, "Unknown entity type during serialization");
                continue;
        }

        // Now we can safely get the buffer
        auto entity_data = entity_builder.GetBufferPointer();
        auto entity_data_size = entity_builder.GetSize();
        auto entity_data_vec = builder.CreateVector(entity_data, entity_data_size);

        // Get creation source info
        EntityCreationSource source = EntityCreationSource_MANUAL;
        flatbuffers::Offset<flatbuffers::String> factory_path_offset = 0;

        auto creation_it = m_entity_creation_sources.find(entity->URI());
        if (creation_it != m_entity_creation_sources.end()) {
            source = creation_it->second.source;
            if (source == EntityCreationSource_FACTORY) {
                factory_path_offset = builder.CreateString(creation_it->second.factory_path.path());
            }
        }

        // Create PersistedEntity
        auto persisted = CreatePersistedEntity(
            builder,
            static_cast<uint8_t>(entity->serialized_entity_type()),
            entity_data_vec,
            source,
            factory_path_offset
        );
        persisted_entities.push_back(persisted);
    }

    // Create hierarchy with persisted entities
    auto persisted_entities_vec = builder.CreateVector(persisted_entities);
    return CreateHierarchy(builder, 0, 0, persisted_entities_vec).o;
}

void ZstStageHierarchy::deserialize_partial(const void* buffer) {
    throw std::runtime_error("Partial deserialization not supported for Hierarchy");
}

void ZstStageHierarchy::deserialize(const Hierarchy* hierarchy_data) {
    if (!hierarchy_data) {
        throw std::runtime_error("Invalid hierarchy data");
    }

    // Clear existing hierarchy first before deserializing
    reset();

    try {
        // Prefer persisted_entities if available (new format with creation source)
        if (hierarchy_data->persisted_entities() && hierarchy_data->persisted_entities()->size() > 0) {
            // First pass: Create offline performer proxies for all unique performer URIs
            std::set<ZstURI> performer_uris;
            for (auto persisted : *hierarchy_data->persisted_entities()) {
                auto entity_data_vec = persisted->entity_data();
                auto entity_type = static_cast<EntityTypes>(persisted->entity_type());

                // Extract entity URI to determine which performer it belongs to
                const EntityData* entity_field = nullptr;
                switch (entity_type) {
                    case EntityTypes_Component:
                        entity_field = flatbuffers::GetRoot<Component>(entity_data_vec->data())->entity();
                        break;
                    case EntityTypes_Plug:
                        entity_field = flatbuffers::GetRoot<Plug>(entity_data_vec->data())->entity();
                        break;
                    case EntityTypes_Factory:
                        entity_field = flatbuffers::GetRoot<Factory>(entity_data_vec->data())->entity();
                        break;
                    default:
                        continue;
                }

                if (entity_field) {
                    ZstURI entity_path(entity_field->URI()->c_str(), entity_field->URI()->size());
                    performer_uris.insert(entity_path.first());
                }
            }

            // Create offline performer proxies
            for (const auto& performer_uri : performer_uris) {
                // Create a minimal Performer FlatBuffer for the proxy
                FlatBufferBuilder perf_builder;
                auto entity_data = CreateEntityData(perf_builder,
                    perf_builder.CreateString(performer_uri.path()),
                    0  // No owner
                );
                auto component_data = CreateComponentData(perf_builder,
                    perf_builder.CreateString("")  // Empty component type
                );
                auto performer_data = CreatePerformerData(perf_builder);
                auto performer_fb = CreatePerformer(perf_builder, entity_data, component_data, performer_data);
                perf_builder.Finish(performer_fb);
                auto performer_root = flatbuffers::GetRoot<Performer>(perf_builder.GetBufferPointer());

                // Create proxy with null transport (offline)
                auto performer_proxy = std::make_unique<ZstPerformerStageProxy>(
                    performer_root,
                    "",  // reliable_address
                    "",  // reliable_public_address
                    "",  // unreliable_address
                    "",  // unreliable_public_address
                    boost::uuids::nil_uuid(),  // origin_endpoint_UUID
                    std::weak_ptr<ZstStageTransport>()  // origin_transport
                );
                ZstHierarchy::add_proxy_entity(std::move(performer_proxy));

                // Set OFFLINE status AFTER adding to hierarchy
                auto added_performer = find_entity(performer_uri);
                if (added_performer) {
                    synchronisable_set_activation_status(added_performer, ZstSyncStatus::OFFLINE);
                    m_offline_entities[performer_uri].push_back(added_performer);
                    Log::server(Log::Level::debug, "Created offline performer proxy {} with status {}",
                        performer_uri.path(), static_cast<int>(added_performer->activation_status()));
                }
            }

            // Second pass: Load all entities
            for (auto persisted : *hierarchy_data->persisted_entities()) {
                // Deserialize entity from nested finished buffer
                auto entity_data_vec = persisted->entity_data();
                auto entity_type = static_cast<EntityTypes>(persisted->entity_type());

                // The entity_data is a finished FlatBuffer, so we need to get the root based on type
                const void* entity_data = nullptr;
                const EntityData* entity_field = nullptr;

                switch (entity_type) {
                    case EntityTypes_Component:
                        entity_data = flatbuffers::GetRoot<Component>(entity_data_vec->data());
                        entity_field = static_cast<const Component*>(entity_data)->entity();
                        break;
                    case EntityTypes_Performer:
                        entity_data = flatbuffers::GetRoot<Performer>(entity_data_vec->data());
                        entity_field = static_cast<const Performer*>(entity_data)->entity();
                        break;
                    case EntityTypes_Plug:
                        entity_data = flatbuffers::GetRoot<Plug>(entity_data_vec->data());
                        entity_field = static_cast<const Plug*>(entity_data)->entity();
                        break;
                    case EntityTypes_Factory:
                        entity_data = flatbuffers::GetRoot<Factory>(entity_data_vec->data());
                        entity_field = static_cast<const Factory*>(entity_data)->entity();
                        break;
                    default:
                        Log::server(Log::Level::error, "Unknown entity type during deserialization");
                        continue;
                }

                auto entity_path = ZstURI(entity_field->URI()->c_str(), entity_field->URI()->size());

                // Create entity
                std::unique_ptr<ZstEntityBase> entity = create_proxy_entity(entity_type, entity_field, entity_data);
                ZstEntityBase* entity_ptr = entity.get();

                // Mark as OFFLINE since owner is not connected
                synchronisable_set_activation_status(entity_ptr, ZstSyncStatus::OFFLINE);

                ZstHierarchy::add_proxy_entity(std::move(entity));

                // Store creation source info
                if (persisted->creation_source() == EntityCreationSource_FACTORY && persisted->factory_path()) {
                    ZstURI factory_path(persisted->factory_path()->c_str(), persisted->factory_path()->size());
                    m_entity_creation_sources[entity_path] = EntityCreationInfo(EntityCreationSource_FACTORY, factory_path);
                } else {
                    m_entity_creation_sources[entity_path] = EntityCreationInfo(EntityCreationSource_MANUAL);
                }

                // Add to offline entities map grouped by performer
                ZstURI performer_uri = entity_path.first();
                m_offline_entities[performer_uri].push_back(entity_ptr);

                Log::server(Log::Level::debug, "Loaded entity {} as OFFLINE ({})", entity_path.path(),
                    (persisted->creation_source() == EntityCreationSource_FACTORY) ? "FACTORY" : "MANUAL");
            }
        }
        // Fallback to old format for backwards compatibility
        else if (hierarchy_data->entities() && hierarchy_data->entities()->size() > 0) {
            Log::server(Log::Level::warn, "Loading session in legacy format (no creation source tracking)");
            for (uoffset_t i = 0; i < hierarchy_data->entities()->size(); i++) {
                EntityTypes entity_type = static_cast<EntityTypes>(hierarchy_data->entities_type()->Get(i));
                const void* entity_data = hierarchy_data->entities()->Get(i);
                const EntityData* entity_field = get_entity_field(entity_type, entity_data);
                auto entity_path = ZstURI(entity_field->URI()->c_str(), entity_field->URI()->size());

                std::unique_ptr<ZstEntityBase> entity = create_proxy_entity(entity_type, entity_field, entity_data);
                ZstEntityBase* entity_ptr = entity.get();
                synchronisable_set_activation_status(entity_ptr, ZstSyncStatus::OFFLINE);
                ZstHierarchy::add_proxy_entity(std::move(entity));

                // Default to MANUAL for legacy sessions
                m_entity_creation_sources[entity_path] = EntityCreationInfo(EntityCreationSource_MANUAL);

                ZstURI performer_uri = entity_path.first();
                m_offline_entities[performer_uri].push_back(entity_ptr);
            }
        }
    } catch (const std::exception& e) {
        Log::server(Log::Level::error, "Error deserializing hierarchy: {}", e.what());
        throw;
    }
}

Signal ZstStageHierarchy::signal_handler(const std::shared_ptr<ZstStageMessage>& request, ZstPerformerStageProxy* sender)
{
    if (!sender) {
        return Signal_ERR_STAGE_PERFORMER_NOT_FOUND;
    }

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

    // Check if there are offline entities for this performer name
    bool has_offline = has_offline_entities(client_URI);

    // Only one client with this UUID at a time (unless it's offline and being reclaimed)
    auto existing = find_entity(client_URI);
    if (existing && existing->activation_status() != ZstSyncStatus::OFFLINE) {
        Log::server(Log::Level::warn, "Client already exists {}", client_URI.path());
        return Signal_ERR_STAGE_PERFORMER_ALREADY_EXISTS;
    }

    if (has_offline) {
        Log::server(Log::Level::notification, "Client {} reconnecting - has offline entities to reclaim", client_URI.path());
    } else {
        Log::server(Log::Level::notification, "Registering new client {}", client_URI.path());
    }

    // Create proxy
    if (auto transport = std::dynamic_pointer_cast<ZstStageTransport>(request->owning_transport())) {
        ZstPerformerStageProxy* performer_ptr = nullptr;

        if (has_offline && existing) {
            // Reuse existing offline performer - just update its transport info
            performer_ptr = dynamic_cast<ZstPerformerStageProxy*>(existing);
            if (performer_ptr) {
                performer_ptr->update_transport_info(
                    (content->graph_reliable_address()) ? content->graph_reliable_address()->str() : "",
                    (content->graph_reliable_public_address()) ? content->graph_reliable_public_address()->str() : "",
                    (content->graph_unreliable_address()) ? content->graph_unreliable_address()->str() : "",
                    (content->graph_unreliable_public_address()) ? content->graph_unreliable_public_address()->str() : "",
                    request->origin_endpoint_UUID(),
                    std::static_pointer_cast<ZstStageTransport>(transport)
                );
                synchronisable_set_activation_status(performer_ptr, ZstSyncStatus::ACTIVATED);
            }
        } else {
            // Create new performer proxy
            std::unique_ptr<ZstPerformerStageProxy> performer = std::make_unique<ZstPerformerStageProxy>(
                content->performer(),
                (content->graph_reliable_address()) ? content->graph_reliable_address()->str() : "",
                (content->graph_reliable_public_address()) ? content->graph_reliable_public_address()->str() : "",
                (content->graph_unreliable_address()) ? content->graph_unreliable_address()->str() : "",
                (content->graph_unreliable_public_address()) ? content->graph_unreliable_public_address()->str() : "",
                request->origin_endpoint_UUID(),
                std::static_pointer_cast<ZstStageTransport>(transport)
            );
            performer_ptr = performer.get();
            ZstHierarchy::add_proxy_entity(std::move(performer));
        }

        // Announce new entity
        if (performer_ptr) {
            dispatch_entity_arrived_event(performer_ptr);

            // If there are offline entities, send notification after registration
            if (has_offline) {
                send_offline_entities_notification(performer_ptr);
            }
        }
    }

    return Signal_OK;
}

Signal ZstStageHierarchy::client_leaving_handler(const std::shared_ptr<ZstStageMessage>& request, ZstPerformerStageProxy* sender)
{
    // Handle performer leaving broadcast
    auto content = request->buffer()->content_as_ClientLeaveRequest();
    
    client_leaving(sender, content->reason());
    return Signal_OK;
}

Signal ZstStageHierarchy::create_entity_handler(const std::shared_ptr<ZstStageMessage>& request, ZstPerformerStageProxy* sender)
{
    auto content = request->buffer()->content_as_EntityCreateRequest();

    for(uoffset_t i = 0; i < content->entity_type()->size(); i++){
        EntityTypes entity_type = static_cast<EntityTypes>(content->entity_type()->Get(i));
        const void* entity_data = content->entity()->Get(i);
        const EntityData* entity_field = get_entity_field(entity_type, entity_data);
        auto entity_path = ZstURI(entity_field->URI()->c_str(), entity_field->URI()->size());

        Log::server(Log::Level::notification, "Activating new proxy entity {}", entity_path.path());
        if (sender->URI().first() != entity_path.first()) {
            //A performer is requesting this entity be attached to another performer
            Log::server(Log::Level::warn, "TODO: Performer requesting new entity to be attached to another performer", entity_path.path());
            return Signal_ERR_ENTITY_NOT_FOUND;
        }

        // Check if this entity already exists as an offline entity
        auto existing = find_entity(entity_path);
        ZstEntityBase* entity_ptr = nullptr;

        if (existing && existing->activation_status() == ZstSyncStatus::OFFLINE) {
            // Auto-reclaim: entity is being recreated with same URI, activate the offline proxy
            Log::server(Log::Level::notification, "Auto-reclaiming offline entity {}", entity_path.path());
            synchronisable_set_activation_status(existing, ZstSyncStatus::ACTIVATED);
            entity_ptr = existing;

            // Remove from offline entities map
            ZstURI performer_uri = entity_path.first();
            auto offline_it = m_offline_entities.find(performer_uri);
            if (offline_it != m_offline_entities.end()) {
                auto& offline_list = offline_it->second;
                offline_list.erase(
                    std::remove(offline_list.begin(), offline_list.end(), existing),
                    offline_list.end()
                );
                if (offline_list.empty()) {
                    m_offline_entities.erase(offline_it);
                }
            }

            // Broadcast entity status change (reactivation)
            transfer_entity_to_performer(existing, sender);

            // Invoke callback so session can sync cables for this reclaimed entity
            if (m_entity_reclaimed_callback) {
                m_entity_reclaimed_callback(existing, sender);
            }
        } else {
            // Create new proxy entity
            std::unique_ptr<ZstEntityBase> entity = create_proxy_entity(entity_type, entity_field, entity_data);
            entity_ptr = entity.get();
            ZstHierarchy::add_proxy_entity(std::move(entity));
            ZstEntityBase* proxy = find_entity(entity_path);
            if (!proxy) {
                Log::server(Log::Level::warn, "No proxy entity found");
                return Signal_ERR_ENTITY_NOT_FOUND;
            }
        }

        // Track creation source - check if this was a factory-created entity
        auto pending_it = m_pending_factory_entities.find(entity_path);
        if (pending_it != m_pending_factory_entities.end()) {
            // This entity was created by a factory
            ZstURI creatable_path = pending_it->second;
            ZstURI factory_path = creatable_path.parent();
            m_entity_creation_sources[entity_path] = EntityCreationInfo(EntityCreationSource_FACTORY, creatable_path);
            m_pending_factory_entities.erase(pending_it);
            Log::server(Log::Level::debug, "Entity {} tracked as FACTORY-created from {}", entity_path.path(), creatable_path.path());
        } else {
            // Manual creation
            m_entity_creation_sources[entity_path] = EntityCreationInfo(EntityCreationSource_MANUAL);
            Log::server(Log::Level::debug, "Entity {} tracked as MANUAL-created", entity_path.path());
        }

        dispatch_entity_arrived_event(entity_ptr);
    }

    return Signal_OK;
}

Signal ZstStageHierarchy::factory_create_entity_handler(const std::shared_ptr<ZstStageMessage>& request, ZstPerformerStageProxy* sender)
{
    auto content = request->buffer()->content_as_FactoryCreateEntityRequest();
    auto creatable_path = ZstURI(content->creatable_entity_URI()->c_str(), content->creatable_entity_URI()->size());
    auto factory_path = creatable_path.parent();
    
    Log::server(Log::Level::notification, "Forwarding creatable entity request {}", creatable_path.path());

    ZstEntityFactory* factory = dynamic_cast<ZstEntityFactory*>(find_entity(factory_path));
    if (!factory) {
        Log::server(Log::Level::error, "Could not find factory {}", factory_path.path());
        return Signal_ERR_ENTITY_NOT_FOUND;
    }

    //Find the performer that owns the factory
    ZstPerformerStageProxy* factory_performer = dynamic_cast<ZstPerformerStageProxy*>(find_entity(factory_path.first()));

    //Check to see if one client is already connected to the other
    if (!factory_performer){
        Log::server(Log::Level::error, "Could not find factory {}", factory_path.path());
        return Signal_ERR_STAGE_PERFORMER_NOT_FOUND;
    }

    // Register pending factory entity creation for tracking
    m_pending_factory_entities[creatable_path] = creatable_path;

    //Send creatable message to the performer that owns the factory
    ZstTransportArgs args;
    args.msg_send_behaviour = ZstTransportRequestBehaviour::ASYNC_REPLY;
    args.on_recv_response = [this, sender, factory_path, creatable_path, response_id = request->id()](ZstMessageResponse response) {
        if (!ZstStageTransport::verify_signal(response.response, Signal_OK, "Creatable request at origin")) {
            reply_with_signal(sender, ZstStageTransport::get_signal(response.response), response_id);
            // Remove from pending if failed
            m_pending_factory_entities.erase(creatable_path);
            return;
        }

        Log::server(Log::Level::notification, "Remote factory created entity {}", factory_path.path());

        // Send ACK to original sender with the path of our new entity
        ZstTransportArgs ack_args;
        ack_args.msg_ID = response_id;
        FlatBufferBuilder builder;
        auto create_entity_ACK = CreateFactoryCreateEntityACK(builder, builder.CreateString(factory_path.path()));
        whisper(sender, Content_FactoryCreateEntityACK, create_entity_ACK.Union(), builder, ack_args);
    };

    //Send creation request to owning factory
    FlatBufferBuilder builder;
    auto create_entity_request = CreateFactoryCreateEntityRequest(builder, builder.CreateString(content->creatable_entity_URI()->str()), builder.CreateString(content->name()->str()));
    whisper(factory_performer, Content_FactoryCreateEntityRequest, create_entity_request.Union(), builder, args);

    return Signal_EMPTY;
}

Signal ZstStageHierarchy::update_entity_handler(const std::shared_ptr<ZstStageMessage>& request, ZstPerformerStageProxy* sender)
{
    auto content = request->buffer()->content_as_EntityUpdateRequest();
    // For serialisation later
    FlatBufferBuilder builder;
    auto entity_field = get_entity_field(content->entity_type(), content->entity());
    auto entity_path = ZstURI(entity_field->URI()->c_str(), entity_field->URI()->size());
    auto original_path = ZstURI(content->original_path()->c_str(), content->original_path()->size());

    Log::server(Log::Level::notification, "Updating proxy entity {}", entity_path.path());
    auto proxy = find_entity(original_path);
    
    if (proxy) {
        ZstHierarchy::update_proxy_entity(proxy, content->entity_type(), entity_field, content->entity());

        auto excluded = std::vector<ZstPerformer*>{ sender };
        ZstTransportArgs args;
        args.msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;
        auto entity_msg = CreateEntityUpdateRequest(
            builder, 
            content->entity_type(),
            proxy->serialize(builder), 
            builder.CreateString(content->original_path()->c_str(), content->original_path()->size())
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

    Log::server(Log::Level::notification, "Removing proxy entity {}", entity_path.path());

    //Remove the entity
    ZstHierarchy::remove_proxy_entity(entity);

    //Update rest of network first
    auto excluded = std::vector<ZstPerformer*>{ sender };
    ZstTransportArgs args;
    args.msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;
    FlatBufferBuilder builder;
    auto destroy_msg_offset = CreateEntityDestroyRequest(builder, builder.CreateString(content->URI()->str()));
    broadcast(Content_EntityDestroyRequest, destroy_msg_offset.Union(), builder, args, excluded);

    return Signal_OK;
}

void ZstStageHierarchy::request_entity_registration(ZstEntityBase* entity)
{
    register_entity(entity);
}

void ZstStageHierarchy::reply_with_signal(ZstPerformerStageProxy* performer, Signal signal, ZstMsgID request_id)
{
    FlatBufferBuilder builder;
    ZstTransportArgs args;
    args.msg_ID = request_id;
    auto signal_offset = CreateSignalMessage(builder, signal);
    whisper(performer, Content_SignalMessage, signal_offset.Union(), builder, args);
}

void ZstStageHierarchy::broadcast(showtime::Content message_content_type, flatbuffers::Offset<void> message_content, flatbuffers::FlatBufferBuilder& builder, const ZstTransportArgs& args, const std::vector<ZstPerformer*> & excluded)
{
    std::unordered_map<ZstStageTransport*, flatbuffers::DetachedBuffer> cached_messages;

    // Get every performer that we're going to broadcast to
    ZstEntityBundle bundle;
    get_performers(&bundle);
    for (auto entity : bundle)
    {
        //Can only send messages to performers
        ZstPerformerStageProxy* performer = dynamic_cast<ZstPerformerStageProxy*>(entity);
        if (!performer || std::find(excluded.begin(), excluded.end(), performer) != excluded.end()) {
            continue;
        }

        // Each performer might have a different transport type so we have to cache different message buffers 
        if (auto transport = performer->origin_transport().lock()) {
            flatbuffers::DetachedBuffer message_buffer;

            // Retrieve cached message for our transport type or create if missing
            auto msg = cached_messages.find(transport.get());
            if (msg != cached_messages.end()) {
                whisper(performer, std::forward<flatbuffers::DetachedBuffer>(msg->second), args);
            }
            else {
                cached_messages[transport.get()] = transport->create_msg(message_content_type, message_content, builder);
                whisper(performer, std::forward<flatbuffers::DetachedBuffer>(cached_messages[transport.get()]), args);
            }
        }        
    }
}

void ZstStageHierarchy::whisper(ZstPerformerStageProxy* performer, showtime::Content message_content_type, flatbuffers::Offset<void> message_content, flatbuffers::FlatBufferBuilder& builder, const ZstTransportArgs& args)
{
    ZstTransportArgs endpoint_args = args;
    endpoint_args.target_endpoint_UUID = performer->origin_endpoint_UUID();

    if (auto transport = performer->origin_transport().lock())
        transport->send_msg(std::forward<flatbuffers::DetachedBuffer>(transport->create_msg(message_content_type, message_content, builder)), endpoint_args);
}

void ZstStageHierarchy::whisper(ZstPerformerStageProxy* performer, flatbuffers::DetachedBuffer&& message_buffer, const ZstTransportArgs& args)
{
    ZstTransportArgs endpoint_args = args;
    endpoint_args.target_endpoint_UUID = performer->origin_endpoint_UUID();

    if (auto transport = performer->origin_transport().lock())
        transport->send_msg(std::forward<flatbuffers::DetachedBuffer>(message_buffer), endpoint_args);
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
    return NULL;
}

// ----------------
// Offline entity management
// ----------------

void ZstStageHierarchy::set_preserve_entities_on_disconnect(bool preserve)
{
    m_preserve_entities_on_disconnect = preserve;
}

bool ZstStageHierarchy::get_preserve_entities_on_disconnect() const
{
    return m_preserve_entities_on_disconnect;
}

void ZstStageHierarchy::set_entity_reclaimed_callback(EntityReclaimedCallback callback)
{
    m_entity_reclaimed_callback = callback;
}

void ZstStageHierarchy::mark_performer_offline(ZstPerformer* performer)
{
    if (!performer)
        return;

    Log::server(Log::Level::notification, "Marking performer {} as OFFLINE (preserving entities)", performer->URI().path());

    // Collect all entities belonging to this performer
    std::vector<ZstEntityBase*> offline_list;
    ZstEntityBundle bundle;
    performer->get_child_entities(&bundle, true, true);

    // Mark all child entities as OFFLINE
    for (auto entity : bundle) {
        synchronisable_set_activation_status(entity, ZstSyncStatus::OFFLINE);
        offline_list.push_back(entity);
    }

    // Mark performer itself as OFFLINE
    synchronisable_set_activation_status(performer, ZstSyncStatus::OFFLINE);
    offline_list.push_back(performer);

    // Store in offline entities map
    m_offline_entities[performer->URI()] = offline_list;

    // Broadcast entity status change to other clients (entity went offline)
    auto excluded = std::vector<ZstPerformer*>{ performer };
    ZstTransportArgs args;
    args.msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;
    FlatBufferBuilder builder;
    auto leave_msg = CreateClientLeaveRequest(builder, builder.CreateString(performer->URI().path()), ClientLeaveReason_PRESERVE_OFFLINE);
    broadcast(Content_ClientLeaveRequest, leave_msg.Union(), builder, args, excluded);
}

bool ZstStageHierarchy::has_offline_entities(const ZstURI& performer_uri) const
{
    return m_offline_entities.find(performer_uri) != m_offline_entities.end();
}

void ZstStageHierarchy::get_offline_entities_for_performer(const ZstURI& performer_uri, ZstEntityBundle& bundle) const
{
    auto it = m_offline_entities.find(performer_uri);
    if (it != m_offline_entities.end()) {
        for (auto entity : it->second) {
            bundle.add(entity);
        }
    }
}

void ZstStageHierarchy::send_offline_entities_notification(ZstPerformerStageProxy* performer)
{
    if (!performer)
        return;

    ZstEntityBundle bundle;
    get_offline_entities_for_performer(performer->URI(), bundle);

    if (bundle.size() == 0)
        return;

    Log::server(Log::Level::notification, "Sending offline entities notification to {} ({} entities)",
        performer->URI().path(), bundle.size());

    // Serialize entities
    FlatBufferBuilder builder;
    std::vector<uint8_t> entity_types;
    std::vector<uint8_t> entity_union_types;
    std::vector<flatbuffers::Offset<void>> entities_serialized;

    for (auto entity : bundle) {
        auto serialized_type = entity->serialized_entity_type();
        entity_types.push_back(static_cast<uint8_t>(serialized_type));
        entity_union_types.push_back(static_cast<uint8_t>(serialized_type));
        entities_serialized.push_back(entity->serialize(builder));
    }

    // Also include cables involving these entities
    // TODO: Include cables in notification

    auto notification = CreateOfflineEntitiesNotification(
        builder,
        builder.CreateString(performer->URI().path()),
        builder.CreateVector(entity_types),
        builder.CreateVector(entity_union_types),
        builder.CreateVector(entities_serialized),
        0  // offline_cables
    );

    ZstTransportArgs args;
    whisper(performer, Content_OfflineEntitiesNotification, notification.Union(), builder, args);
}

void ZstStageHierarchy::transfer_entity_to_performer(ZstEntityBase* entity, ZstPerformerStageProxy* new_owner)
{
    if (!entity || !new_owner)
        return;

    Log::server(Log::Level::notification, "Transferring entity {} ownership to {}",
        entity->URI().path(), new_owner->URI().path());

    // Mark entity as active again
    synchronisable_set_activation_status(entity, ZstSyncStatus::ACTIVATED);

    // Broadcast entity status change
    ZstTransportArgs args;
    args.msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;
    FlatBufferBuilder builder;

    std::vector<uint8_t> entity_types;
    std::vector<flatbuffers::Offset<void>> entities_serialized;
    entity_types.push_back(static_cast<uint8_t>(entity->serialized_entity_type()));
    entities_serialized.push_back(entity->serialize(builder));

    auto content_message = CreateEntityCreateRequest(
        builder,
        builder.CreateVector(entity_types),
        builder.CreateVector(entities_serialized)
    );

    auto excluded = std::vector<ZstPerformer*>{ new_owner };
    broadcast(Content_EntityCreateRequest, content_message.Union(), builder, args, excluded);
}

Signal ZstStageHierarchy::entity_reclaim_handler(const std::shared_ptr<ZstStageMessage>& request, ZstPerformerStageProxy* sender)
{
    if (!sender)
        return Signal_ERR_STAGE_PERFORMER_NOT_FOUND;

    auto content = request->buffer()->content_as_EntityReclaimRequest();

    Log::server(Log::Level::notification, "Received entity reclaim request from {}", sender->URI().path());

    for (auto uri_fb : *content->entity_URIs()) {
        auto uri = ZstURI(uri_fb->c_str(), uri_fb->size());
        auto entity = find_entity(uri);

        if (entity && entity->activation_status() == ZstSyncStatus::OFFLINE) {
            transfer_entity_to_performer(entity, sender);
            Log::server(Log::Level::notification, "Entity {} reclaimed by {}", uri.path(), sender->URI().path());
        } else {
            Log::server(Log::Level::warn, "Entity {} not found or not offline, cannot reclaim", uri.path());
        }
    }

    // Remove from offline entities map
    m_offline_entities.erase(sender->URI());

    return Signal_OK;
}

} // namespace showtime
