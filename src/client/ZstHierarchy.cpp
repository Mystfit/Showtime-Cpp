#include "ZstHierarchy.h"

ZstHierarchy::ZstHierarchy(ZstClient * client) : 
	m_root(NULL),
	ZstClientModule(client)
{

	m_performer_arriving_event_manager = new ZstEventQueue();
	m_component_arriving_event_manager = new ZstEventQueue();
	m_plug_arriving_event_manager = new ZstEventQueue();
	m_performer_leaving_event_manager = new ZstEventQueue();
	m_component_leaving_event_manager = new ZstEventQueue();
	m_plug_leaving_event_manager = new ZstEventQueue();

	m_performer_leaving_hook = new ZstEntityLeavingEvent();
	m_component_leaving_hook = new ZstEntityLeavingEvent();
	m_plug_leaving_hook = new ZstPlugLeavingEvent();
	m_performer_leaving_event_manager->attach_post_event_callback(m_performer_leaving_hook);
	m_component_leaving_event_manager->attach_post_event_callback(m_component_leaving_hook);
	m_plug_leaving_event_manager->attach_post_event_callback(m_plug_leaving_hook);

	add_event_queue(m_performer_arriving_event_manager);
	add_event_queue(m_component_arriving_event_manager);
	add_event_queue(m_plug_arriving_event_manager);
	add_event_queue(m_performer_leaving_event_manager);
	add_event_queue(m_component_leaving_event_manager);
	add_event_queue(m_plug_leaving_event_manager);
}

ZstHierarchy::~ZstHierarchy()
{
	m_performer_leaving_event_manager->remove_post_event_callback(m_performer_leaving_hook);
	m_component_leaving_event_manager->remove_post_event_callback(m_component_leaving_hook);
	m_plug_leaving_event_manager->remove_post_event_callback(m_plug_leaving_hook);

	remove_event_queue(m_performer_leaving_event_manager);
	remove_event_queue(m_component_leaving_event_manager);
	remove_event_queue(m_plug_leaving_event_manager);

	remove_event_queue(m_performer_arriving_event_manager);
	remove_event_queue(m_component_arriving_event_manager);
	remove_event_queue(m_plug_arriving_event_manager);

	delete m_performer_leaving_hook;
	delete m_component_leaving_hook;
	delete m_plug_leaving_hook;
	delete m_performer_leaving_event_manager;
	delete m_component_leaving_event_manager;
	delete m_plug_leaving_event_manager;
	delete m_performer_arriving_event_manager;
	delete m_component_arriving_event_manager;
	delete m_plug_arriving_event_manager;
}

void ZstHierarchy::destroy()
{
	//TODO: Delete proxies and templates
	delete m_root;
}

void ZstHierarchy::init(std::string name)
{
	//Create a root entity to hold our local entity hierarchy
	//Sets the name of our performer and the address of our graph output
	m_root = new ZstPerformer(name.c_str());
	m_root->set_network_interactor(client()->msg_dispatch());
}

void ZstHierarchy::synchronise_graph(bool async)
{
	if (!client()->is_connected_to_stage()) {
		ZstLog::net(LogLevel::error, "Can't synchronise graph if we're not connected");
		return;
	}

	//Ask the stage to send us a full snapshot
	ZstLog::net(LogLevel::notification, "Requesting stage snapshot");

	ZstMessage * msg = client()->msg_dispatch()->init_message(ZstMsgKind::CLIENT_SYNC);
	client()->msg_dispatch()->send_to_stage(msg, [this](ZstMessageReceipt response) { this->synchronise_graph_complete(response); }, async);
}

void ZstHierarchy::synchronise_graph_complete(ZstMessageReceipt response)
{
	m_root->enqueue_activation();
	ZstLog::net(LogLevel::notification, "Graph sync completed");
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
	entity->set_network_interactor(client());
	entity->set_activating();
	
	//Build message
	ZstMessage * msg = client()->msg_dispatch()->init_entity_message(entity);
	client()->msg_dispatch()->send_to_stage(msg, async, [this, entity](ZstMessageReceipt response) { this->activate_entity_complete(response, entity); });
}


void ZstHierarchy::activate_entity_complete(ZstMessageReceipt response, ZstEntityBase * entity)
{
	if (response.status != ZstMsgKind::OK) {
		ZstLog::net(LogLevel::error, "Activate entity {} failed with status {}", entity->URI().path(), response.status);
		return;
	}

	if (response.status == ZstMsgKind::OK) {
		entity->enqueue_activation();
	}
	else if (response.status == ZstMsgKind::ERR_STAGE_ENTITY_ALREADY_EXISTS) {
		entity->set_error(ZstSyncError::ENTITY_ALREADY_EXISTS);
	}
	else if (response.status == ZstMsgKind::ERR_STAGE_ENTITY_NOT_FOUND) {
		entity->set_error(ZstSyncError::PERFORMER_NOT_FOUND);
	}
	else if (response.status == ZstMsgKind::ERR_STAGE_ENTITY_NOT_FOUND) {
		entity->set_error(ZstSyncError::PARENT_NOT_FOUND);
	}

	ZstLog::net(LogLevel::notification, "Activate entity {} complete with status {}", entity->URI().path(), response.status);
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
		if (client()->is_connected_to_stage()) {
			ZstURI entity_path = entity->URI();
			ZstMessage * msg = client()->msg_dispatch()->init_message(ZstMsgKind::DESTROY_ENTITY);
			msg->append_str(entity->URI().path(), entity->URI().full_size());
			client()->msg_dispatch()->send_to_stage(msg, async, [this, entity](ZstMessageReceipt response) { this->destroy_entity_complete(response, entity); });

			if (async) {
				//Since we own this entity, we can start to clean it up immediately
				entity->enqueue_deactivation();
				component_leaving_events()->enqueue(entity);
				process_callbacks();
			}
			else {
				destroy_entity_sync(entity, future);
			}
		}
		else {
			entity->enqueue_deactivation();
			component_leaving_events()->enqueue(entity);
			process_callbacks();
		}
	}
}

void ZstHierarchy::destroy_entity_complete(ZstMessageReceipt response, ZstEntityBase * entity)
{
	if (!entity) return;

	entity->set_destroyed();

	if (response.status != ZstMsgKind::OK) {
		ZstLog::net(LogLevel::notification, "Destroy entity failed with status {}", response.status);
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
		client()->enqueue_synchronisable_deletion(entity);
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

void ZstHierarchy::destroy_plug_complete(ZstMessageReceipt response, ZstPlug * plug)
{
	plug->set_destroyed();

	ZstComponent * parent = dynamic_cast<ZstComponent*>(plug->parent());
	parent->remove_plug(plug);

	//Finally, add to the reaper to destroy the plug at the correct time
	client()->enqueue_synchronisable_deletion(plug);
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
			component_arriving_events()->enqueue(entity_proxy);
		}
		else if (strcmp(entity.entity_type(), CONTAINER_TYPE) == 0) {
			entity_proxy = new ZstContainer(static_cast<ZstContainer&>(entity));
			dynamic_cast<ZstContainer*>(parent)->add_child(entity_proxy);
			component_arriving_events()->enqueue(entity_proxy);
		}
		else if (strcmp(entity.entity_type(), PLUG_TYPE) == 0) {
			ZstPlug * plug = new ZstPlug(static_cast<ZstPlug&>(entity));
			entity_proxy = plug;
			dynamic_cast<ZstComponent*>(parent)->add_plug(plug);
			plug_arriving_events()->enqueue(entity_proxy);
		}
		else {
			ZstLog::net(LogLevel::notification, "Can't create unknown proxy entity type {}", entity.entity_type());
		}

		ZstLog::net(LogLevel::notification, "Received proxy entity {}", entity_proxy->URI().path());

		//Forceably activate entity and dispatch events
		entity_proxy->set_network_interactor(client());
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

ZstEventQueue * ZstHierarchy::performer_arriving_events()
{
	return m_performer_arriving_event_manager;
}

ZstEventQueue * ZstHierarchy::performer_leaving_events()
{
	return m_performer_leaving_event_manager;
}

ZstEventQueue * ZstHierarchy::component_arriving_events()
{
	return m_component_arriving_event_manager;
}

ZstEventQueue * ZstHierarchy::component_leaving_events()
{
	return m_component_leaving_event_manager;
}

ZstEventQueue * ZstHierarchy::component_type_arriving_events()
{
	return m_component_type_arriving_event_manager;
}

ZstEventQueue * ZstHierarchy::component_type_leaving_events()
{
	return m_component_type_leaving_event_manager;
}

ZstEventQueue * ZstHierarchy::plug_arriving_events()
{
	return m_plug_arriving_event_manager;
}

ZstEventQueue * ZstHierarchy::plug_leaving_events()
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
		performer_arriving_events()->enqueue(performer_proxy);
	}
}


// -----------------------------
// Sync/async block to convert
// -----------------------------


//void ZstClient::activate_entity_sync(ZstEntityBase * entity, MessageFuture & future)
//{
//	try {
//		ZstMsgKind status = future.get();
//		activate_entity_complete(status, entity);
//		process_callbacks();
//	}
//	catch (const ZstTimeoutException & e) {
//		ZstLog::net(LogLevel::notification, "Activate entity sync call timed out: {}", e.what());
//	}
//}
//
//void ZstClient::activate_entity_async(ZstEntityBase * entity, MessageFuture & future)
//{
//	ZstURI entity_path(entity->URI());
//	future.then([this, entity_path](MessageFuture f) {
//		ZstMsgKind status(ZstMsgKind::EMPTY);
//		try {
//			status = f.get();
//			ZstEntityBase * e = find_entity(entity_path);
//			if (e)
//				this->activate_entity_complete(status, e);
//			else
//				ZstLog::net(LogLevel::notification, "Entity {} went missing during activation!", entity_path.path());
//		}
//		catch (const ZstTimeoutException & e) {
//			ZstLog::net(LogLevel::notification, "Activate entity async call timed out: {}", e.what());
//			status = ZstMsgKind::ERR_STAGE_TIMEOUT;
//		}
//		return status;
//	});
//}

//void ZstClient::destroy_entity_sync(ZstEntityBase * entity, MessageFuture & future)
//{
//	try {
//		ZstMsgKind status = future.get();
//		entity->enqueue_deactivation();
//		component_leaving_events().enqueue(entity);
//		process_callbacks();
//	}
//	catch (const ZstTimeoutException & e) {
//		ZstLog::net(LogLevel::notification, "Destroy entity sync timed out: {}", e.what());
//	}
//}
//
//void ZstClient::destroy_entity_async(ZstEntityBase * entity, MessageFuture & future)
//{
//	ZstURI entity_path(entity->URI());
//	future.then([this, entity_path](MessageFuture f) {
//		ZstMsgKind status(ZstMsgKind::EMPTY);
//		try {
//			status = f.get();
//			if (status != ZstMsgKind::OK) {
//				ZstLog::net(LogLevel::notification, "Destroy entity {} failed with status {}", entity_path.path(), status);
//			}
//			ZstLog::net(LogLevel::notification, "Destroy entity {} completed with status {}", entity_path.path(), status);
//		}
//		catch (const ZstTimeoutException & e) {
//			ZstLog::net(LogLevel::notification, "Destroy entity async timed out: {}", e.what());
//			status = ZstMsgKind::ERR_STAGE_TIMEOUT;
//		}
//		return status;
//	});
//}

//void ZstClient::destroy_plug_sync(ZstPlug * plug, MessageFuture & future)
//{
//	try {
//		ZstMsgKind status = future.get();
//		destroy_plug_complete(status, plug);
//		process_callbacks();
//	}
//	catch (const ZstTimeoutException & e) {
//		ZstLog::net(LogLevel::notification, "Destroy plug timed out: {}", e.what());
//	}
//}
//
//void ZstClient::destroy_plug_async(ZstPlug * plug, MessageFuture & future)
//{
//	try {
//		future.then([this, plug](MessageFuture f) {
//			ZstMsgKind status = f.get();
//			this->destroy_plug_complete(status, plug);
//			return status;
//		});
//	}
//	catch (const ZstTimeoutException & e) {
//		ZstLog::net(LogLevel::notification, "Destroy plug timed out: {}", e.what());
//	}
//}
