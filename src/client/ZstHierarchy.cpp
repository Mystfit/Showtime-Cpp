#include "ZstHierarchy.h"

ZstHierarchy::ZstHierarchy() : 
	m_root(NULL)
{
	m_performer_leaving_hook = new ZstEntityLeavingEvent();
	m_component_leaving_hook = new ZstEntityLeavingEvent();
	m_plug_leaving_hook = new ZstPlugLeavingEvent();
}

ZstHierarchy::~ZstHierarchy()
{
}

void ZstHierarchy::activate_entity(ZstEntityBase * entity, bool async)
{
	//If the entity doesn't have a parent, put it under the root container
	if (!entity->parent()) {
		ZstLog::net(LogLevel::notification, "No parent set for {}, adding to {}", entity->URI().path(), m_root->URI().path());
		m_root->add_child(entity);
	}

	//If this is not a local entity, we can't activate it
	if (!entity_is_local(*entity))
		return;

	//Register client to entity to allow it to send messages
	entity->set_network_interactor(this);
	entity->set_activating();

	//Build message
	ZstMessage * msg = msg_pool().get()->init_entity_message(entity);
	ZstURI entity_path = entity->URI();

	MessageFuture future = msg_pool().register_response_message(msg, true);
	if (async) {
		activate_entity_async(entity, future);
		send_to_stage(msg);
	}
	else {
		send_to_stage(msg);
		activate_entity_sync(entity, future);
	}
}


void ZstHierarchy::destroy_entity(ZstEntityBase * entity, bool async)
{
	if (!entity) {
		return;
	}

	//Set entity state as deactivating so we can't access it further
	entity->set_deactivating();

	//If the entity is local, let the stage know it's leaving
	if (entity_is_local(*entity)) {
		if (is_connected_to_stage()) {
			ZstMessage * msg = msg_pool().get()->init_message(ZstMsgKind::DESTROY_ENTITY);
			msg->append_str(entity->URI().path(), entity->URI().full_size());
			ZstURI entity_path = entity->URI();

			MessageFuture future = msg_pool().register_response_message(msg, true);
			if (async) {
				destroy_entity_async(entity, future);
				send_to_stage(msg);

				//Since we own this entity, we can start to clean it up immediately
				entity->enqueue_deactivation();
				component_leaving_events().enqueue(entity);
				process_callbacks();
			}
			else {
				send_to_stage(msg);
				destroy_entity_sync(entity, future);
			}

		}
		else {
			entity->enqueue_deactivation();
			component_leaving_events().enqueue(entity);
			process_callbacks();
		}
	}
}


void ZstHierarchy::destroy_plug(ZstPlug * plug, bool async)
{
	if (!plug) {
		return;
	}

	plug->set_deactivating();

	if (entity_is_local(*plug)) {
		ZstMessage * msg = msg_pool().get()->init_message(ZstMsgKind::DESTROY_ENTITY);
		msg->append_str(plug->URI().path(), plug->URI().full_size());
		MessageFuture future = msg_pool().register_future(msg, true);
		if (async) {
			destroy_plug_async(plug, future);
			send_to_stage(msg);
		}
		else {
			send_to_stage(msg);
			destroy_plug_sync(plug, future);
		}
	}
}


ZstEntityBase * ZstHierarchy::find_entity(const ZstURI & path)
{
	ZstEntityBase * result = NULL;
	ZstPerformer * root = NULL;

	if (path_is_local(path)) {
		//Performer is local
		root = m_root;
	}
	else {
		//Performer is remote
		root = get_performer_by_URI(path);
	}

	if (!root)
		return result;

	if (root->URI() == path) {
		//Root is the entity we're searching for
		return root;
	}

	if (root) {
		//Find child entity in root
		result = root->find_child_by_URI(path);
	}

	return result;
}

ZstPlug * ZstHierarchy::find_plug(const ZstURI & path)
{
	ZstComponent * plug_parent = dynamic_cast<ZstComponent*>(find_entity(path.parent()));
	return plug_parent->get_plug_by_URI(path);
}




bool ZstHierarchy::entity_is_local(ZstEntityBase & entity)
{
	return path_is_local(entity.URI());
}

bool ZstHierarchy::path_is_local(const ZstURI & path) {
	return path.contains(m_root->URI());
}

void ZstHierarchy::add_proxy_entity(ZstEntityBase & entity) {

	//Don't need to activate local entities, they will auto-activate when the stage responds with an OK
	if (entity_is_local(entity)) {
		ZstLog::net(LogLevel::notification, "Received local entity {}. Ignoring", entity.URI().path());
		return;
	}

	ZstURI parent_URI = entity.URI().parent();
	if (parent_URI.size()) {
		ZstEntityBase * parent = find_entity(parent_URI);

		if (find_entity(entity.URI())) {
			ZstLog::net(LogLevel::error, "Can't create entity {}, it already exists", entity.URI().path());
			return;
		}

		ZstEntityBase * entity_proxy = NULL;

		//Create proxies and set parents
		if (strcmp(entity.entity_type(), COMPONENT_TYPE) == 0) {
			entity_proxy = new ZstComponent(static_cast<ZstComponent&>(entity));
			dynamic_cast<ZstContainer*>(parent)->add_child(entity_proxy);
			component_arriving_events().enqueue(entity_proxy);
		}
		else if (strcmp(entity.entity_type(), CONTAINER_TYPE) == 0) {
			entity_proxy = new ZstContainer(static_cast<ZstContainer&>(entity));
			dynamic_cast<ZstContainer*>(parent)->add_child(entity_proxy);
			component_arriving_events().enqueue(entity_proxy);
		}
		else if (strcmp(entity.entity_type(), PLUG_TYPE) == 0) {
			ZstPlug * plug = new ZstPlug(static_cast<ZstPlug&>(entity));
			entity_proxy = plug;
			dynamic_cast<ZstComponent*>(parent)->add_plug(plug);
			plug_arriving_events().enqueue(entity_proxy);
		}
		else {
			ZstLog::net(LogLevel::notification, "Can't create unknown proxy entity type {}", entity.entity_type());
		}

		ZstLog::net(LogLevel::notification, "Received proxy entity {}", entity_proxy->URI().path());

		//Forceably activate entity and dispatch events
		entity_proxy->set_network_interactor(this);
		entity_proxy->set_activation_status(ZstSyncStatus::ACTIVATED);
	}

}


ZstPerformer * ZstHierarchy::get_performer_by_URI(const ZstURI & uri) const
{
	ZstPerformer * result = NULL;
	ZstURI performer_URI = uri.first();

	auto entity_iter = m_clients.find(performer_URI);
	if (entity_iter != m_clients.end()) {
		result = entity_iter->second;
	}

	return result;
}


ZstPerformer * ZstHierarchy::get_local_performer() const
{
	return m_root;
}



ZstEventQueue & ZstHierarchy::performer_arriving_events()
{
	return m_performer_arriving_event_manager;
}

ZstEventQueue & ZstHierarchy::performer_leaving_events()
{
	return m_performer_leaving_event_manager;
}

ZstEventQueue & ZstHierarchy::component_arriving_events()
{
	return m_component_arriving_event_manager;
}

ZstEventQueue & ZstHierarchy::component_leaving_events()
{
	return m_component_leaving_event_manager;
}

ZstEventQueue & ZstHierarchy::component_type_arriving_events()
{
	return m_component_type_arriving_event_manager;
}

ZstEventQueue & ZstHierarchy::component_type_leaving_events()
{
	return m_component_type_leaving_event_manager;
}

ZstEventQueue & ZstHierarchy::plug_arriving_events()
{
	return m_plug_arriving_event_manager;
}

ZstEventQueue & ZstHierarchy::plug_leaving_events()
{
	return m_plug_leaving_event_manager;
}



void ZstHierarchy::add_performer(ZstPerformer & performer)
{
	if (performer.URI() == m_root->URI()) {
		ZstLog::net(LogLevel::debug, "Received self {} as performer. Ignoring", m_root->URI().path());
		return;
	}

	//Copy streamable so we have a local ptr for the performer
	ZstPerformer * performer_proxy = new ZstPerformer(performer);
	assert(performer_proxy);
	ZstLog::net(LogLevel::notification, "Adding new performer {}", performer_proxy->URI().path());

	//Since this is a proxy entity, it should be activated immediately.
	performer_proxy->set_activation_status(ZstSyncStatus::ACTIVATED);

	if (performer.URI() != m_root->URI()) {
		m_clients[performer_proxy->URI()] = performer_proxy;
		performer_proxy->set_activation_status(ZstSyncStatus::ACTIVATED);
		performer_arriving_events().enqueue(performer_proxy);
	}
}


void ZstHierarchy::activate_entity_complete(ZstMsgKind status, ZstEntityBase * entity)
{
	if (status != ZstMsgKind::OK) {
		ZstLog::net(LogLevel::error, "Activate entity {} failed with status {}", entity->URI().path(), status);
		return;
	}

	if (status == ZstMsgKind::OK) {
		entity->enqueue_activation();
	}
	else if (status == ZstMsgKind::ERR_STAGE_ENTITY_ALREADY_EXISTS) {
		entity->set_error(ZstSyncError::ENTITY_ALREADY_EXISTS);
	}
	else if (status == ZstMsgKind::ERR_STAGE_ENTITY_NOT_FOUND) {
		entity->set_error(ZstSyncError::PERFORMER_NOT_FOUND);
	}
	else if (status == ZstMsgKind::ERR_STAGE_ENTITY_NOT_FOUND) {
		entity->set_error(ZstSyncError::PARENT_NOT_FOUND);
	}

	ZstLog::net(LogLevel::notification, "Activate entity {} complete with status {}", entity->URI().path(), status);
}


void ZstHierarchy::destroy_entity_complete(ZstMsgKind status, ZstEntityBase * entity)
{
	if (entity) {
		entity->set_destroyed();

		if (status != ZstMsgKind::OK) {
			ZstLog::net(LogLevel::notification, "Destroy entity failed with status {}", status);
		}

		//Remove entity from parent
		if (entity->parent()) {
			ZstContainer * parent = dynamic_cast<ZstContainer*>(entity->parent());
			parent->remove_child(entity);
			entity->m_parent = NULL;
		}
		else {
			//Entity is a root performer. Remove from performer list
			m_clients.erase(entity->URI());
		}

		//Finally, add non-local entities to the reaper to destroy them at the correct time
		//TODO: Only destroying proxy entities at the moment. Local entities should be managed by the host application
		if (!entity_is_local(*entity))
			m_reaper.add(entity);
	}
}

void ZstHierarchy::destroy_plug_complete(ZstMsgKind status, ZstPlug * plug)
{
	plug->set_destroyed();

	ZstComponent * parent = dynamic_cast<ZstComponent*>(plug->parent());
	parent->remove_plug(plug);

	//Finally, add to the reaper to destroy the plug at the correct time
	m_reaper.add(plug);
}
