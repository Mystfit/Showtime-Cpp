#include "ZstClientHierarchy.h"

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
        m_root->get_child_entities(bundle, true);
        for (auto entity : bundle) {
            destroy_entity_complete(entity);
        }
    }
}

void ZstClientHierarchy::init(std::string name)
{
	ZstHierarchy::init();

	//Create a root entity to hold our local entity hierarchy
	//Sets the name of our performer and the address of our graph output
    m_root = std::make_shared<ZstPerformer>(name.c_str());
	m_root->add_adaptor(ZstSynchronisableAdaptor::downcasted_shared_from_this<ZstSynchronisableAdaptor>());
}

void ZstClientHierarchy::destroy()
{
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

void ZstClientHierarchy::on_receive_msg(const ZstStageMessage * stage_msg)
{
    switch (stage_msg->type()) {
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

void ZstClientHierarchy::on_publish_entity_update(ZstEntityBase * entity)
{
	if (entity->entity_type(), EntityType_FACTORY) {
		//Factory wants to update creatables
		ZstEntityFactory * factory = static_cast<ZstEntityFactory*>(entity);
        
		stage_events()->invoke([factory](std::shared_ptr<ZstStageTransportAdaptor> adaptor) {
            // Create transport args
			ZstTransportArgs args;
			args.msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;
            
            // Serialize factory into buffer
            FlatBufferBuilder builder;
            EntityBuilder entity_builder(builder);
            auto factory_offset = factory->serialize(entity_builder);
            std::vector< flatbuffers::Offset<Entity> > factory_vec{ factory_offset };
            
            // Create message
            auto update_builder = EntityUpdateRequestBuilder(builder);
            update_builder.add_entities(builder.CreateVector(factory_vec));
            auto update_msg = update_builder.Finish();
            
            // Send message
            adaptor->send_msg(Content_EntityUpdateRequest, update_msg.Union(), builder);
		});
	}
}

void ZstClientHierarchy::on_request_entity_activation(ZstEntityBase * entity)
{
	activate_entity(entity, ZstTransportRequestBehaviour::SYNC_REPLY, [](ZstMessageReceipt receipt) {});
}

void ZstClientHierarchy::activate_entity(ZstEntityBase * entity, const ZstTransportRequestBehaviour & sendtype)
{
	activate_entity(entity, sendtype, [](ZstMessageReceipt receipt) {});
}

void ZstClientHierarchy::activate_entity(ZstEntityBase * entity, const ZstTransportRequestBehaviour & sendtype, ZstMessageReceivedAction callback)
{
	if(!entity){
		ZstLog::net(LogLevel::error, "Can't activate a null entity");
		return;
	}

	//If the entity doesn't have a parent, put it under the root container
	if (!entity->parent()) {
        ZstLog::net(LogLevel::warn, "{} has no parent", entity->URI().path());
        return;
	}
	 
    //Super activation
	ZstHierarchy::activate_entity(entity, sendtype);
	
	//Build message
	ZstTransportArgs args;
	args.msg_send_behaviour = sendtype;
	args.on_recv_response = [this, entity, callback](ZstMessageReceipt response) {
		if (response.status == Signal_OK){
			this->activate_entity_complete(entity);
			callback(response);
		}
		else {
			ZstLog::net(LogLevel::error, "Activate entity {} failed with status {}", entity->URI().path(), EnumNameSignal(response.status));
			return;
		}
	};

	//Send message
	stage_events()->invoke([args, entity](std::shared_ptr<ZstStageTransportAdaptor> adaptor){
        auto builder = FlatBufferBuilder();
        
        // Serialize entity
        auto entity_builder = EntityBuilder(builder);
        auto entity_vec = builder.CreateVector(std::vector<flatbuffers::Offset<Entity> >{entity->serialize(entity_builder)});

        // Create content message
        auto content_message = CreateEntityCreateRequest(builder, entity_vec);
        
        // Send message
        adaptor->send_msg(Content_EntityCreateRequest, content_message.Union(), builder);
	});

	if (sendtype == ZstTransportRequestBehaviour::SYNC_REPLY)
		process_events();
}


void ZstClientHierarchy::destroy_entity(ZstEntityBase * entity, const ZstTransportRequestBehaviour & sendtype)
{
	if (!entity) return;

	ZstHierarchy::destroy_entity(entity, sendtype);

	//If the entity is local, let the stage know it's leaving
	if (!entity->is_proxy()) {
		//Build message
		ZstTransportArgs args;
		args.msg_send_behaviour = sendtype;
		args.on_recv_response = [this, entity](ZstMessageReceipt response) {
			if (response.status != Signal_OK) {
				ZstLog::net(LogLevel::error, "Destroy entity failed with status {}", EnumNameSignal(response.status));
				return;
			}
			this->destroy_entity_complete(entity);
		};

		//Send message
		stage_events()->invoke([this, &args, entity](std::shared_ptr<ZstStageTransportAdaptor> adaptor) {
            
            FlatBufferBuilder builder;
            auto content_message = CreateEntityDestroyRequest(builder, builder.CreateString(entity->URI().path(), entity->URI().full_size()));
            adaptor->send_msg(Content_EntityDestroyRequest, content_message.Union(), builder);
            
			if (args.msg_send_behaviour == ZstTransportRequestBehaviour::PUBLISH) {
				this->destroy_entity_complete(entity);
			}
		});
	}
	else {
		destroy_entity_complete(entity);
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
		ZstLog::net(LogLevel::warn, "Could not find factory to create entity {}", creatable_path.path());
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
    stage_events()->invoke([this, sendtype, creatable_path, &entity, entity_name, factory](std::shared_ptr<ZstStageTransportAdaptor> adaptor) {
		ZstTransportArgs args;
		args.msg_send_behaviour = sendtype;
		args.on_recv_response = [this, &entity, sendtype, creatable_path, entity_name, factory](ZstMessageReceipt response) {
			if (response.status != Signal_OK) {
				ZstLog::net(LogLevel::error, "Creating remote entity from factory failed with status {}", EnumNameSignal(response.status));
				return;
			}

			ZstLog::net(LogLevel::notification, "Created entity from {}", creatable_path.path());
			if (sendtype == ZstTransportRequestBehaviour::SYNC_REPLY) {
				//Can return the entity since the pointer reference will still be on the stack
				entity = find_entity(creatable_path.first() + ZstURI(entity_name));
			}
			if (sendtype == ZstTransportRequestBehaviour::ASYNC_REPLY) {
				ZstEntityBase* late_entity = find_entity(creatable_path.first() + ZstURI(entity_name));
				if (late_entity) {
					factory->factory_events()->defer([late_entity](std::shared_ptr<ZstFactoryAdaptor> adaptor) { 
						adaptor->on_entity_created(late_entity);
					});
					factory->synchronisable_events()->invoke([factory](std::shared_ptr<ZstSynchronisableAdaptor> adaptor) {
						adaptor->on_synchronisable_has_event(factory);
					});
				}
			}
		};
        
        FlatBufferBuilder builder;
        auto content_msg = CreateFactoryCreateEntityRequest(builder, builder.CreateString(creatable_path.path(), creatable_path.full_size()),  builder.CreateString(entity_name.path(), entity_name.full_size()));
        adaptor->send_msg(Content_FactoryCreateEntityRequest, content_msg.Union(), builder, args);
    });

	return entity;
}
    
void ZstClientHierarchy::create_proxy_entity_handler(const EntityCreateRequest * request)
{
    for (auto it = request->entities_type()->begin(); it != request->entities_type()->end();  ++it )
    {
        auto index = it - request->entities_type()->begin();
        auto entity_type = static_cast<EntityTypes>(*it);
        
        switch(entity_type){
            case EntityTypes_Performer:
                add_performer(request->entities()->GetAs<Performer>(index));
                break;
            case EntityTypes_Component:
                add_proxy_entity();
                break;
            case EntityTypes_Factory:
                break;
            case EntityTypes_Plug:
                break;
            case EntityTypes_NONE:
                break;
        }
    }
}
    
void ZstClientHierarchy::update_proxy_entity_handler(const EntityUpdateRequest * request)
{
    throw std::runtime_error("Updating proxy entity not implemented");
    //update_proxy_entity();
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
    
	ZstLog::net(LogLevel::notification, "Received remote request to create a {} entity with the name {} ", creatable_path.path(), name);

	//Find the factory and delegate the entity creation to the main event loop thread
	ZstEntityFactory * factory = dynamic_cast<ZstEntityFactory*>(find_entity(creatable_path.parent()));
	if (!factory) {
		ZstLog::net(LogLevel::warn, "Could not find factory to create entity {}", creatable_path.path());
		return;
	}
	factory->synchronisable_events()->defer([this, creatable_path, name, factory, request_id](std::shared_ptr<ZstSynchronisableAdaptor> adaptor) {
		ZstEntityBase * entity = factory->create_entity(creatable_path, name.c_str());
		if (entity) {
            // Add entity to local performer
            this->get_local_performer()->add_child(entity, false);
			ZstLog::net(LogLevel::notification, "Activating creatable {} ", entity->URI().path());
            
            // Activate entity separately
			this->activate_entity(entity, ZstTransportRequestBehaviour::ASYNC_REPLY, [this, entity, request_id](ZstMessageReceipt receipt) {
				ZstLog::net(LogLevel::notification, "Creatable {} activated", entity->URI().path());

				stage_events()->invoke([request_id](std::shared_ptr<ZstStageTransportAdaptor> adaptor) {
					ZstTransportArgs args;
                    args.msg_ID = request_id;
                    
                    // Send signal
                    auto builder = FlatBufferBuilder();
                    auto signal = CreateSignalMessage(builder, Signal_OK);
                    adaptor->send_msg(Content_SignalMessage, signal.Union(), builder, args);
				});
			});
		}
		else {
			stage_events()->invoke([request_id](std::shared_ptr<ZstStageTransportAdaptor> adaptor) {
				ZstTransportArgs args;
                args.msg_ID = request_id;
                
                // Send signals
                auto builder = FlatBufferBuilder();
                auto signal = CreateSignalMessage(builder, Signal_ERR_ENTITY_NOT_FOUND);
                adaptor->send_msg(Content_SignalMessage, signal.Union(), builder, args);
			});
		}
	});

	// Signal main event loop that the factory has an event waiting
	factory->synchronisable_events()->invoke([factory](std::shared_ptr<ZstSynchronisableAdaptor> adaptor) { 
		adaptor->on_synchronisable_has_event(factory);
	});
}

void ZstClientHierarchy::activate_entity_complete(ZstEntityBase * entity)
{
	ZstHierarchy::activate_entity_complete(entity);

	ZstEntityBundle bundle;
    entity->get_child_entities(bundle, false);
	for (auto c : bundle) {
		hierarchy_events()->invoke([c](std::shared_ptr<ZstHierarchyAdaptor> adaptor) { 
			adaptor->on_entity_arriving(c); 
		});
	}
}

void ZstClientHierarchy::destroy_entity_complete(ZstEntityBase * entity)
{
	if (!entity) {
		ZstLog::net(LogLevel::warn, "destroy_entity_complete(): Entity not found");
		return;
	}

	if (entity->URI() == this->get_local_performer()->URI()) {
		ZstLog::net(LogLevel::debug, "Destroyed entity is our own client, ignore.");
		return;
	}
	ZstHierarchy::destroy_entity_complete(entity);
}


ZstEntityBase * ZstClientHierarchy::find_entity(const ZstURI & path) const
{
	if (m_root->URI() == path) {
		return m_root.get();
	}
	return ZstHierarchy::find_entity(path);
}

bool ZstClientHierarchy::path_is_local(const ZstURI & path) {
	return path.contains(m_root->URI());
}

void ZstClientHierarchy::add_proxy_entity(const Entity* entity) {

	// Don't need to activate local entities, they will auto-activate when the stage responds with an OK
	// Also, we can't rely on the proxy flag here as it won't have been set yet
    auto entity_path = ZstURI(entity->URI()->c_str(), entity->URI()->size());
	if (path_is_local(entity_path)) {
		ZstLog::net(LogLevel::debug, "Received local entity {}. Ignoring", entity_path.path());
		return;
    } else {
        ZstHierarchy::add_proxy_entity(entity);
    }
    
    //Dispatch entity arrived event regardless if the entity is local or remote
    dispatch_entity_arrived_event(find_entity(entity_path));
}

void ZstClientHierarchy::update_proxy_entity(const Entity* entity)
{
    auto entity_path = ZstURI(entity->URI()->c_str(), entity->URI()->size());
    
	//Don't need to update local entities, they should have published the update
	if (path_is_local(entity_path)) {
		ZstLog::net(LogLevel::debug, "Don't need to update a local entity {}. Ignoring", entity_path.path());
		return;
	}
    
	ZstHierarchy::update_proxy_entity(entity);
}

ZstPerformer * ZstClientHierarchy::get_local_performer() const
{
	return m_root.get();
}

void ZstClientHierarchy::add_performer(const Performer* entity)
{
    auto performer_path = ZstURI(entity->URI()->c_str(), entity->URI()->size());
    
	if (performer_path == m_root->URI()) {
		//If we received ourselves as a performer, then we are now activated and can be added to the entity lookup map
		ZstEntityBundle bundle;
		m_root->get_child_entities(bundle, true);
		m_root->get_factories(bundle);
		for (auto c : bundle) {
			add_entity_to_lookup(c);
		}
		ZstLog::net(LogLevel::debug, "Received self {} as performer. Caching in entity lookup", m_root->URI().path());
		return;
	}

	ZstHierarchy::add_performer(entity);
}

ZstEntityBundle & ZstClientHierarchy::get_performers(ZstEntityBundle & bundle) const
{
	//TODO: Add local performer to the main client list?
	//Join local performer to the performer list since it lives outside the main list
	bundle.add(m_root.get());
	return ZstHierarchy::get_performers(bundle);
}

ZstPerformer * ZstClientHierarchy::get_performer_by_URI(const ZstURI & uri) const
{
	if (uri.first() == m_root->URI()) {
		return m_root.get();
	}
	return ZstHierarchy::get_performer_by_URI(uri);
}

}
