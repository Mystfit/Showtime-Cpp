#include "ZstClientHierarchy.h"
#include <boost/assign.hpp>
#include <boost/lexical_cast.hpp>

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
	switch (msg->kind()) {
	case ZstMsgKind::CREATE_PLUG:
		add_proxy_entity(msg->unpack_payload_serialisable<ZstPlug>());
		break;
	case ZstMsgKind::CREATE_PERFORMER:
		add_performer(msg->unpack_payload_serialisable<ZstPerformer>());
		break;
	case ZstMsgKind::CREATE_COMPONENT:
		add_proxy_entity(msg->unpack_payload_serialisable<ZstComponent>());
		break;
	case ZstMsgKind::CREATE_CONTAINER:
		add_proxy_entity(msg->unpack_payload_serialisable<ZstContainer>());
		break;
	case ZstMsgKind::CREATE_ENTITY_FROM_FACTORY:
		create_entity_handler(msg);
		break;
	case ZstMsgKind::DESTROY_ENTITY:
		destroy_entity_complete(find_entity(ZstURI(msg->get_arg(ZstMsgArg::PATH))));
		break;
	default:
		break;
	}
}

void ZstClientHierarchy::on_publish_entity_update(ZstEntityBase * entity)
{
	performance_events().invoke([entity](ZstTransportAdaptor * adaptor) {
		ZstOutputPlug * plug = dynamic_cast<ZstOutputPlug*>(entity);
		if (plug) {
			ZstMsgArgs args = { { ZstMsgArg::PATH, plug->URI().path() } };
			if (!plug->is_reliable())
				args[ZstMsgArg::UNRELIABLE] = "";
			adaptor->on_send_msg(ZstMsgKind::PERFORMANCE_MSG, args, *plug->raw_value());
		}
	});
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
			args[ZstMsgArg::MSG_ID] = boost::lexical_cast<std::string>(request_ID);
		}
		ZstLog::net(LogLevel::debug, "Responding to server creatable request with id {}", request_ID);
		adaptor->on_send_msg(ZstMessage::entity_kind(*entity), sendtype, *entity, args, [this, entity](ZstMessageReceipt response) {
			if (response.status == ZstMsgKind::CREATE_COMPONENT ||
				response.status == ZstMsgKind::CREATE_CONTAINER ||
				response.status == ZstMsgKind::CREATE_FACTORY ||
				response.status == ZstMsgKind::CREATE_PERFORMER) {
				this->activate_entity_complete(entity);
			}
			else {
				ZstLog::net(LogLevel::error, "Activate entity {} failed with status {}", entity->URI().path(), ZstMsgNames[response.status]);
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
			adaptor->on_send_msg(ZstMsgKind::DESTROY_ENTITY, sendtype, {{ZstMsgArg::PATH, entity->URI().path()}}, [this, entity](ZstMessageReceipt response) {
				if (response.status != ZstMsgKind::OK) {
					ZstLog::net(LogLevel::error, "Destroy entity failed with status {}", ZstMsgNames[response.status]);
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
		stage_events().invoke([this, sendtype, creatable_path, &entity, entity_name](ZstTransportAdaptor * adaptor) {
			ZstMsgArgs args{  
				{ ZstMsgArg::PATH, creatable_path.path() },
				{ ZstMsgArg::NAME, entity_name.path() }
			};
			adaptor->on_send_msg(ZstMsgKind::CREATE_ENTITY_FROM_FACTORY, sendtype, args, [this, &entity, sendtype, creatable_path, entity_name](ZstMessageReceipt response) {
				if (response.status == ZstMsgKind::CREATE_COMPONENT ||
					response.status == ZstMsgKind::CREATE_CONTAINER ||
					response.status == ZstMsgKind::CREATE_FACTORY) 
				{
					ZstLog::net(LogLevel::notification, "Created entity from {}", creatable_path.path());
					if (sendtype == ZstTransportSendType::SYNC_REPLY) {
						//Can return the entity since the pointer reference will still be on the stack
						entity = find_entity(creatable_path.first() + ZstURI(entity_name));
					}
				}
				else {
					ZstLog::net(LogLevel::error, "Creating remote entity from factory failed with status {}", ZstMsgNames[response.status]);
					return;
				}
			});
		});
	}
	else {
		//Internal factory - create entity locally
		ZstLog::net(LogLevel::notification, "Received remote request to create a {} entity with the name {} ", creatable_path.path(), name);
		entity = ZstHierarchy::create_entity(creatable_path, name, sendtype);

		//If we're creating a local entity, make sure to activate it afterwards
		if(activate)
			activate_entity(entity, sendtype);
	}

	return entity;
}

void ZstClientHierarchy::create_entity_handler(ZstMessage * msg)
{
	ZstMsgKind status(ZstMsgKind::OK);
	ZstEntityBase * entity = create_entity(ZstURI(msg->get_arg(ZstMsgArg::PATH)), msg->get_arg(ZstMsgArg::NAME), false);

	//Defer entity activation till now so that we can forward the message ID back to the stage
	if (entity) {
		activate_entity(entity, ZstTransportSendType::ASYNC_REPLY, msg->id());
	}
	else {
		stage_events().invoke([msg, status](ZstTransportAdaptor * adp) {
			adp->on_send_msg(ZstMsgKind::ERR_ENTITY_NOT_FOUND, { { ZstMsgArg::MSG_ID, boost::lexical_cast<std::string>(msg->id()) } });
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
