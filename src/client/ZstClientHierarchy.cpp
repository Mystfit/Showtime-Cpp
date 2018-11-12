#include "ZstClientHierarchy.h"

ZstClientHierarchy::ZstClientHierarchy() :
	m_root(NULL)
{
}

ZstClientHierarchy::~ZstClientHierarchy()
{
}

void ZstClientHierarchy::init(std::string name)
{
	ZstHierarchy::init();

	//Create a root entity to hold our local entity hierarchy
	//Sets the name of our performer and the address of our graph output
	m_root = new ZstPerformer(name.c_str());
	m_root->add_adaptor(static_cast<ZstSynchronisableAdaptor*>(this));
}

void ZstClientHierarchy::destroy()
{
	stage_events().remove_all_adaptors();

	//TODO: Delete other clientsa
	//Reset local performer
	size_t index = 0;

	ZstEntityBundle bundle;
	for (auto entity : m_root->get_child_entities(bundle, true)) {
		destroy_entity_complete(entity);
	}

	//Process events to make sure events are dispatched properly
	//process_events();
	delete m_root;

	ZstHierarchy::destroy();
}

void ZstClientHierarchy::process_events()
{
	ZstHierarchy::process_events();
	stage_events().process_events();
}

void ZstClientHierarchy::flush_events()
{
	ZstHierarchy::flush_events();
	stage_events().flush();
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
	case ZstMsgKind::CREATE_CONTAINER:
		add_proxy_entity(stage_msg->unpack_payload_serialisable<ZstContainer>());
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
		stage_events().invoke([factory](ZstTransportAdaptor * adp) {
			adp->on_send_msg(ZstMsgKind::UPDATE_ENTITY, ZstTransportSendType::PUBLISH, factory->as_json(), json::object(), [](ZstMessageReceipt s) {});
		});
	}
}

void ZstClientHierarchy::activate_entity(ZstEntityBase * entity, const ZstTransportSendType & sendtype)
{
	activate_entity(entity, sendtype, 0);
}

void ZstClientHierarchy::activate_entity(ZstEntityBase * entity, const ZstTransportSendType & sendtype, ZstMsgID request_ID)
{
	if(!entity){
		ZstLog::net(LogLevel::error, "Can't activate a null entity");
		return;
	}

	//If the entity doesn't have a parent, put it under the root container
	if (!entity->parent()) {
		ZstLog::net(LogLevel::debug, "No parent set for {}, adding to {}", entity->URI().path(), m_root->URI().path());
		m_root->add_child(entity);
	}
	 
	ZstHierarchy::activate_entity(entity, sendtype);
	
	//Build message
	stage_events().invoke([this, entity, sendtype, request_ID](ZstTransportAdaptor * adaptor)
	{
		ZstMsgArgs args;
		if (request_ID > 0) {
			args [get_msg_arg_name(ZstMsgArg::MSG_ID)] = request_ID;
			ZstLog::net(LogLevel::debug, "Responding to server creatable request with id {}", request_ID);
		}
		adaptor->on_send_msg(ZstStageMessage::entity_kind(*entity), sendtype, entity->as_json(), args, [this, entity](ZstMessageReceipt response) {
			if (response.status == ZstMsgKind::CREATE_COMPONENT ||
				response.status == ZstMsgKind::CREATE_CONTAINER ||
				response.status == ZstMsgKind::CREATE_FACTORY ||
				response.status == ZstMsgKind::CREATE_PERFORMER || 
				response.status == ZstMsgKind::CREATE_PLUG ||
				response.status == ZstMsgKind::OK) 
			{
				ZstLog::net(LogLevel::debug, "activate_entity(): Server responded with {}", get_msg_name(response.status));
				this->activate_entity_complete(entity);
			} else {
				ZstLog::net(LogLevel::error, "Activate entity {} failed with status {}", entity->URI().path(), get_msg_name(response.status));
				return;
			}
		});
	});

	if (sendtype == ZstTransportSendType::SYNC_REPLY)
		process_events();
}


void ZstClientHierarchy::destroy_entity(ZstEntityBase * entity, const ZstTransportSendType & sendtype)
{
	if (!entity) return;

	ZstHierarchy::destroy_entity(entity, sendtype);

	//If the entity is local, let the stage know it's leaving
	if (!entity->is_proxy()) {
		stage_events().invoke([this, sendtype, entity](ZstTransportAdaptor * adaptor) {
			adaptor->on_send_msg(ZstMsgKind::DESTROY_ENTITY, sendtype, { {get_msg_arg_name(ZstMsgArg::PATH), entity->URI().path()} }, [this, entity](ZstMessageReceipt response) {
				if (response.status != ZstMsgKind::OK) {
					ZstLog::net(LogLevel::error, "Destroy entity failed with status {}", get_msg_name(response.status));
					return;
				}
				this->destroy_entity_complete(entity);
			});
			
			if (sendtype == ZstTransportSendType::PUBLISH) {
				this->destroy_entity_complete(entity);
			}
		});
	}
	else {
		destroy_entity_complete(entity);
	}

	if (sendtype == ZstTransportSendType::SYNC_REPLY) {
		process_events();
	}
}

ZstEntityBase * ZstClientHierarchy::create_entity(const ZstURI & creatable_path, const char * name, bool activate)
{
	return create_entity(creatable_path, name, activate, ZstTransportSendType::ASYNC_REPLY);
}

ZstEntityBase * ZstClientHierarchy::create_entity(const ZstURI & creatable_path, const char * name, bool activate, const ZstTransportSendType & sendtype)
{
	ZstEntityBase * entity = NULL;
	//Find the factory associated with this creatable path
	ZstEntityFactory * factory = dynamic_cast<ZstEntityFactory*>(find_entity(creatable_path.parent()));
	if (!factory) {
		ZstLog::net(LogLevel::warn, "Could not find factory to create entity {}", creatable_path.path());
		return NULL;
	}

	ZstURI entity_name(name);

	//External factory - route creation request
	if (factory->is_proxy()) {
		stage_events().invoke([this, sendtype, creatable_path, &entity, entity_name, factory](ZstTransportAdaptor * adaptor) {
			ZstMsgArgs args{  
				{ get_msg_arg_name(ZstMsgArg::PATH), creatable_path.path() },
				{ get_msg_arg_name(ZstMsgArg::NAME), entity_name.path() }
			};
			adaptor->on_send_msg(ZstMsgKind::CREATE_ENTITY_FROM_FACTORY, sendtype, args, [this, &entity, sendtype, creatable_path, entity_name, factory](ZstMessageReceipt response) {
				if (response.status == ZstMsgKind::CREATE_COMPONENT ||
					response.status == ZstMsgKind::CREATE_CONTAINER ||
					response.status == ZstMsgKind::CREATE_FACTORY) 
				{
					ZstLog::net(LogLevel::notification, "Created entity from {}", creatable_path.path());
					if (sendtype == ZstTransportSendType::SYNC_REPLY) {
						//Can return the entity since the pointer reference will still be on the stack
						entity = find_entity(creatable_path.first() + ZstURI(entity_name));
					}
					if (sendtype == ZstTransportSendType::ASYNC_REPLY) {
						ZstEntityBase * late_entity = find_entity(creatable_path.first() + ZstURI(entity_name));
						if (late_entity) {
							factory->factory_events()->defer([late_entity](ZstFactoryAdaptor * adp) { adp->on_entity_created(late_entity); });
							factory->synchronisable_events()->invoke([factory](ZstSynchronisableAdaptor * adp) { adp->on_synchronisable_has_event(factory); });
						}
					}
				}
				else {
					ZstLog::net(LogLevel::error, "Creating remote entity from factory failed with status {}", get_msg_name(response.status));
					return;
				}
			});
		});
	}
	else {
		entity = ZstHierarchy::create_entity(creatable_path, name, activate, sendtype);
	}

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
	factory->synchronisable_events()->defer([this, creatable_path, name, factory, msg_id](ZstSynchronisableAdaptor * adp) {
		ZstEntityBase * entity = factory->create_entity(creatable_path, name.c_str());
		if (entity) {
			this->activate_entity(entity, ZstTransportSendType::ASYNC_REPLY, msg_id);
		}
		else {
			stage_events().invoke([msg_id](ZstTransportAdaptor * adp) {
				adp->on_send_msg(ZstMsgKind::ERR_ENTITY_NOT_FOUND, { { get_msg_arg_name(ZstMsgArg::MSG_ID), msg_id } });
			});
		}
	});

	//Signal main event loop that the factory has an event waiting
	factory->synchronisable_events()->invoke([factory](ZstSynchronisableAdaptor* adp) { 
		adp->on_synchronisable_has_event(factory); 
	});
}

void ZstClientHierarchy::activate_entity_complete(ZstEntityBase * entity)
{
	ZstHierarchy::activate_entity_complete(entity);

	ZstEntityBundle bundle;
	for (auto c : entity->get_child_entities(bundle, false)) {
		module_events().invoke([c](ZstModuleAdaptor * adp) { adp->on_entity_arriving(c); });
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
		return m_root;
	}
	return ZstHierarchy::find_entity(path);
}

bool ZstClientHierarchy::path_is_local(const ZstURI & path) {
	return path.contains(m_root->URI());
}

ZstMsgKind ZstClientHierarchy::add_proxy_entity(const ZstEntityBase & entity) {

	// Don't need to activate local entities, they will auto-activate when the stage responds with an OK
	// Also, we can't rely on the proxy flag here as it won't have been set yet
	if (path_is_local(entity.URI())) {
		ZstLog::net(LogLevel::debug, "Received local entity {}. Ignoring", entity.URI().path());
		return ZstMsgKind::EMPTY;
	}
	return ZstHierarchy::add_proxy_entity(entity);
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
	return m_root;
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
	bundle.add(m_root);
	return ZstHierarchy::get_performers(bundle);
}

ZstPerformer * ZstClientHierarchy::get_performer_by_URI(const ZstURI & uri) const
{
	if (uri.first() == m_root->URI()) {
		return m_root;
	}
	return ZstHierarchy::get_performer_by_URI(uri);
}
