#include "ZstClientHierarchy.h"

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

void ZstClientHierarchy::on_receive_msg(ZstMessage * msg)
{
	ZstStageMessage * stage_msg = static_cast<ZstStageMessage*>(msg);
	switch (stage_msg->kind()) {
	case ZstMsgKind::CREATE_PLUG:
		add_proxy_entity(stage_msg->unpack_payload_serialisable<ZstPlug>());
		break;
	case ZstMsgKind::CREATE_PERFORMER:
		add_performer(stage_msg->unpack_payload_serialisable<ZstPerformer>());
		break;
	case ZstMsgKind::CREATE_COMPONENT:
		add_proxy_entity(stage_msg->unpack_payload_serialisable<ZstComponent>());
		break;
	case ZstMsgKind::CREATE_ENTITY_FROM_FACTORY:
		create_entity_handler(stage_msg);
		break;
	case ZstMsgKind::UPDATE_ENTITY:
		update_proxy_entity(stage_msg->unpack_payload_serialisable<ZstEntityFactory>());
		break;
	case ZstMsgKind::DESTROY_ENTITY:
	{
		std::string path = stage_msg->get_arg<std::string>(ZstMsgArg::PATH);
		destroy_entity_complete(find_entity(ZstURI(path.c_str(), path.size())));
		break;
	}
	default:
		break;
	}
}

void ZstClientHierarchy::on_publish_entity_update(ZstEntityBase * entity)
{
	if (strcmp(entity->entity_type(), FACTORY_TYPE) == 0) {
		//Factory wants to update creatables
		ZstEntityFactory * factory = static_cast<ZstEntityFactory*>(entity);
		stage_events()->invoke([factory](std::shared_ptr<ZstTransportAdaptor> adaptor) {
			ZstTransportArgs args;
			args.msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;
			factory->write_json(args.msg_payload);
			adaptor->send_msg(ZstMsgKind::UPDATE_ENTITY, args);
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
		//m_root->add_child(entity);
	}
	 
    //Super activation
	ZstHierarchy::activate_entity(entity, sendtype);
	
	//Build message
	auto kind = ZstStageMessage::entity_kind(*entity);
	ZstTransportArgs args;
	//args.msg_args[get_msg_arg_name(ZstMsgArg::MSG_ID)] = request_ID;
	args.msg_send_behaviour = sendtype;
	args.on_recv_response = [this, entity, callback](ZstMessageReceipt response) {
		if (response.status == ZstMsgKind::CREATE_COMPONENT ||
			response.status == ZstMsgKind::CREATE_FACTORY ||
			response.status == ZstMsgKind::CREATE_PERFORMER ||
			response.status == ZstMsgKind::CREATE_PLUG ||
			response.status == ZstMsgKind::OK)
		{
			ZstLog::net(LogLevel::debug, "activate_entity(): Server responded with {}", get_msg_name(response.status));
			this->activate_entity_complete(entity);
			callback(response);
		}
		else {
			ZstLog::net(LogLevel::error, "Activate entity {} failed with status {}", entity->URI().path(), get_msg_name(response.status));
			return;
		}
	};
	entity->write_json(args.msg_payload);

	//Send message
	stage_events()->invoke([kind, args](std::shared_ptr<ZstTransportAdaptor> adaptor){ 
		adaptor->send_msg(kind, args); 
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
		args.msg_args = { {get_msg_arg_name(ZstMsgArg::PATH), entity->URI().path()} };
		args.on_recv_response = [this, entity](ZstMessageReceipt response) {
			if (response.status != ZstMsgKind::OK) {
				ZstLog::net(LogLevel::error, "Destroy entity failed with status {}", get_msg_name(response.status));
				return;
			}
			this->destroy_entity_complete(entity);
		};

		//Send message
		stage_events()->invoke([this, &args, entity](std::shared_ptr<ZstTransportAdaptor> adaptor) {
			adaptor->send_msg(ZstMsgKind::DESTROY_ENTITY, args);
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
    stage_events()->invoke([this, sendtype, creatable_path, &entity, entity_name, factory](std::shared_ptr<ZstTransportAdaptor> adaptor) {
		ZstTransportArgs args;
		args.msg_send_behaviour = sendtype;
		args.msg_args = {
            { get_msg_arg_name(ZstMsgArg::PATH), creatable_path.path() },
            { get_msg_arg_name(ZstMsgArg::NAME), entity_name.path() }
        };
		args.on_recv_response = [this, &entity, sendtype, creatable_path, entity_name, factory](ZstMessageReceipt response) {
			if (response.status != ZstMsgKind::OK) {
				ZstLog::net(LogLevel::error, "Creating remote entity from factory failed with status {}", get_msg_name(response.status));
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

        adaptor->send_msg(ZstMsgKind::CREATE_ENTITY_FROM_FACTORY, args);
    });

	return entity;
}

void ZstClientHierarchy::create_entity_handler(ZstMessage * msg)
{
	//External creation request to create a local entity
	ZstStageMessage * stage_msg = static_cast<ZstStageMessage*>(msg);

	std::string creatable_path_str = stage_msg->get_arg<std::string>(ZstMsgArg::PATH);
	ZstURI creatable_path(creatable_path_str.c_str(), creatable_path_str.size());

	std::string name = stage_msg->get_arg<std::string>(ZstMsgArg::NAME);
	ZstMsgID msg_id = stage_msg->id();
	ZstLog::net(LogLevel::notification, "Received remote request to create a {} entity with the name {} ", creatable_path.path(), name);

	//Find the factory and delegate the entity creation to the main event loop thread
	ZstEntityFactory * factory = dynamic_cast<ZstEntityFactory*>(find_entity(creatable_path.parent()));
	if (!factory) {
		ZstLog::net(LogLevel::warn, "Could not find factory to create entity {}", creatable_path.path());
		return;
	}
	factory->synchronisable_events()->defer([this, creatable_path, name, factory, msg_id](std::shared_ptr<ZstSynchronisableAdaptor> adaptor) {
		ZstEntityBase * entity = factory->create_entity(creatable_path, name.c_str());
		if (entity) {
            //Add entity to local performer
            this->get_local_performer()->add_child(entity, false);

			ZstLog::net(LogLevel::notification, "Activating creatable {} ", entity->URI().path());
            
            //Activate entity separately
			this->activate_entity(entity, ZstTransportRequestBehaviour::ASYNC_REPLY, [this, entity, msg_id](ZstMessageReceipt receipt) {
				ZstLog::net(LogLevel::notification, "Creatable {} activated", entity->URI().path());

				stage_events()->invoke([msg_id](std::shared_ptr<ZstTransportAdaptor> adaptor) {
					ZstTransportArgs args;
					args.msg_args = { { get_msg_arg_name(ZstMsgArg::MSG_ID), msg_id } };
					adaptor->send_msg(ZstMsgKind::OK, args);
				});
			});
		}
		else {
			stage_events()->invoke([msg_id](std::shared_ptr<ZstTransportAdaptor> adaptor) {
				ZstTransportArgs args;
				args.msg_args = { { get_msg_arg_name(ZstMsgArg::MSG_ID), msg_id } };
				adaptor->send_msg(ZstMsgKind::ERR_ENTITY_NOT_FOUND, args);
			});
		}
	});

	//Signal main event loop that the factory has an event waiting
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

ZstMsgKind ZstClientHierarchy::add_proxy_entity(const ZstEntityBase & entity) {

    auto status = ZstMsgKind::EMPTY;
	// Don't need to activate local entities, they will auto-activate when the stage responds with an OK
	// Also, we can't rely on the proxy flag here as it won't have been set yet
	if (path_is_local(entity.URI())) {
		ZstLog::net(LogLevel::debug, "Received local entity {}. Ignoring", entity.URI().path());
		return status;
    } else {
        status = ZstHierarchy::add_proxy_entity(entity);
    }
    
    //Dispatch entity arrived event regardless if the entity is local or remote
    dispatch_entity_arrived_event(find_entity(entity.URI().path()));
    
	return status;
}

ZstMsgKind ZstClientHierarchy::update_proxy_entity(const ZstEntityBase & entity)
{
	//Don't need to update local entities, they should have published the update
	if (path_is_local(entity.URI())) {
		ZstLog::net(LogLevel::debug, "Don't need to update a local entity {}. Ignoring", entity.URI().path());
		return ZstMsgKind::EMPTY;
	}
	return ZstHierarchy::update_proxy_entity(entity);
}

ZstPerformer * ZstClientHierarchy::get_local_performer() const
{
	return m_root.get();
}

void ZstClientHierarchy::add_performer(const ZstPerformer & performer)
{
	if (performer.URI() == m_root->URI()) {
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

	ZstHierarchy::add_performer(performer);
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
