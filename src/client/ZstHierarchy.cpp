#include "ZstHierarchy.h"

ZstHierarchy::ZstHierarchy(ZstClient * client) : 
	m_root(NULL),
	ZstClientModule(client)
{
}

ZstHierarchy::~ZstHierarchy()
{
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
	m_root->add_adaptor(this);
}

void ZstHierarchy::process_events()
{
	ZstEventDispatcher<ZstSynchronisableAdaptor*>::process_events();
	ZstEventDispatcher<ZstSessionAdaptor*>::process_events();
}

void ZstHierarchy::on_receive_from_stage(int payload_index, ZstStageMessage * msg)
{
	switch (msg->payload_at(payload_index).kind()) {
	case ZstMsgKind::CREATE_PLUG:
	{
		ZstPlug plug = msg->unpack_payload_serialisable<ZstPlug>(payload_index);
		add_proxy_entity(plug);
		break;
	}
	case ZstMsgKind::CREATE_PERFORMER:
	{
		ZstPerformer performer = msg->unpack_payload_serialisable<ZstPerformer>(payload_index);
		add_performer(performer);
		break;
	}
	case ZstMsgKind::CREATE_COMPONENT:
	{
		ZstComponent component = msg->unpack_payload_serialisable<ZstComponent>(payload_index);
		add_proxy_entity(component);
		break;
	}
	case ZstMsgKind::CREATE_CONTAINER:
	{
		ZstContainer container = msg->unpack_payload_serialisable<ZstContainer>(payload_index);
		add_proxy_entity(container);
		break;
	}
	case ZstMsgKind::DESTROY_ENTITY:
	{
		ZstURI entity_path = ZstURI((char*)msg->payload_at(payload_index).data(), msg->payload_at(payload_index).size());

		//Only dispatch entity leaving events for non-local entities (for the moment)
		if (!path_is_local(entity_path)) {
			ZstEntityBase * entity = find_entity(entity_path);
			if (entity) {
				synchronisable_enqueue_deactivation(entity);
				if (strcmp(entity->entity_type(), COMPONENT_TYPE) == 0 || strcmp(entity->entity_type(), CONTAINER_TYPE) == 0) {
					destroy_entity(entity, false);
				}
				else if (strcmp(entity->entity_type(), PLUG_TYPE) == 0) {
					destroy_plug(static_cast<ZstPlug*>(entity), false);
				}
				else if (strcmp(entity->entity_type(), PERFORMER_TYPE) == 0) {
					add_event([entity](ZstSessionAdaptor * dlg) {
						dlg->on_performer_leaving(static_cast<ZstPerformer*>(entity)); 
					});
				}
			}
		}

		break;
	}
	default:
		ZstLog::net(LogLevel::notification, "Didn't understand message type of {}", msg->payload_at(payload_index).kind());
		throw std::logic_error("Didn't understand message type");
		break;
	}
}

void ZstHierarchy::notify_event_ready(ZstSynchronisable * synchronisable)
{
	add_event([synchronisable](ZstSessionAdaptor * dlg) { synchronisable->process_events(); });
}

void ZstHierarchy::synchronise_graph(bool async)
{
	//Ask the stage to send us a full snapshot
	ZstLog::net(LogLevel::notification, "Requesting stage snapshot");

	ZstMessage * msg = client()->msg_dispatch()->init_message(ZstMsgKind::CLIENT_SYNC);
	client()->msg_dispatch()->send_to_stage(msg, async, [this](ZstMessageReceipt response) { this->synchronise_graph_complete(response); });
}

void ZstHierarchy::synchronise_graph_complete(ZstMessageReceipt response)
{
	synchronisable_enqueue_activation(m_root);
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
	if (entity->is_proxy())
		return;

	//Register client to entity to allow it to send messages
	entity->add_adaptor(this);
	synchronisable_set_activating(entity);
	
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

	switch (response.status) {
	case ZstMsgKind::OK:
		break;
	case ZstMsgKind::ERR_STAGE_ENTITY_ALREADY_EXISTS:
		synchronisable_set_error(entity, ZstSyncError::ENTITY_ALREADY_EXISTS);
		break;
	case ZstMsgKind::ERR_STAGE_ENTITY_NOT_FOUND:
		synchronisable_set_error(entity, ZstSyncError::PERFORMER_NOT_FOUND);
		break;
	default:
		break;
	}

	ZstLog::net(LogLevel::notification, "Activate entity {} complete with status {}", entity->URI().path(), response.status);
}


void ZstHierarchy::destroy_entity(ZstEntityBase * entity, bool async)
{
	if (!entity) {
		return;
	}

	//Set entity state as deactivating so we can't access it further
	synchronisable_set_deactivating(entity);

	//If the entity is local, let the stage know it's leaving
	if (!entity->is_proxy()) {
		ZstMessage * msg = client()->msg_dispatch()->init_message(ZstMsgKind::DESTROY_ENTITY);
		msg->append_str(entity->URI().path(), entity->URI().full_size());
		client()->msg_dispatch()->send_to_stage(msg, async, [this, entity](ZstMessageReceipt response) { this->destroy_entity_complete(response, entity); });

		if (!async) {				
			process_events();
		}
	}
	else {
		destroy_entity_complete(ZstMessageReceipt{ZstMsgKind::OK, async}, entity);
	}
}

void ZstHierarchy::destroy_entity_complete(ZstMessageReceipt response, ZstEntityBase * entity)
{
	if (!entity) return;

	synchronisable_set_destroyed(entity);

	if (response.status != ZstMsgKind::OK) {
		ZstLog::net(LogLevel::notification, "Destroy entity failed with status {}", response.status);
	}

	//Remove entity from parent
	if (entity->parent()) {
		ZstContainer * parent = dynamic_cast<ZstContainer*>(entity->parent());
		parent->remove_child(entity);
	}
	else {
		//Entity is a root performer. Remove from performer list
		m_clients.erase(entity->URI());
	}

	//Finally, add non-local entities to the reaper to destroy them at the correct time
	//TODO: Only destroying proxy entities at the moment. Local entities should be managed by the host application
	add_event([entity](ZstSessionAdaptor * dlg) {dlg->on_entity_leaving(entity); });
	synchronisable_enqueue_deactivation(entity);
}


void ZstHierarchy::destroy_plug(ZstPlug * plug, bool async)
{
	if (!plug) return;

	synchronisable_set_deactivating(plug);

	if (!plug->is_proxy()) {
		ZstMessage * msg = client()->msg_dispatch()->init_message(ZstMsgKind::DESTROY_ENTITY);
		msg->append_str(plug->URI().path(), plug->URI().full_size());
		client()->msg_dispatch()->send_to_stage(msg, async, [this, plug](ZstMessageReceipt response) {this->destroy_plug_complete(response, plug); });
	}
	else {
		destroy_plug_complete(ZstMessageReceipt{ ZstMsgKind::EMPTY , async}, plug);
	}
}

void ZstHierarchy::destroy_plug_complete(ZstMessageReceipt response, ZstPlug * plug)
{
	synchronisable_set_destroyed(plug);

	ZstComponent * parent = dynamic_cast<ZstComponent*>(plug->parent());
	parent->remove_plug(plug);

	//Queue events
	synchronisable_enqueue_deactivation(plug);
	add_event([plug](ZstSessionAdaptor * dlg){ dlg->on_plug_leaving(plug); });
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

bool ZstHierarchy::path_is_local(const ZstURI & path) {
	return path.contains(m_root->URI());
}

void ZstHierarchy::add_proxy_entity(ZstEntityBase & entity) {

	// Don't need to activate local entities, they will auto-activate when the stage responds with an OK
	// Also, we can't rely on the proxy flag here as it won't have been set yet
	if (path_is_local(entity.URI())) {
		ZstLog::net(LogLevel::notification, "Received local entity {}. Ignoring", entity.URI().path());
		return;
	}

	// All entities need a parent unless they are a performer 
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
			add_event([entity_proxy](ZstSessionAdaptor * dlg) {dlg->on_entity_arriving(entity_proxy); });
		}
		else if (strcmp(entity.entity_type(), CONTAINER_TYPE) == 0) {
			entity_proxy = new ZstContainer(static_cast<ZstContainer&>(entity));
			dynamic_cast<ZstContainer*>(parent)->add_child(entity_proxy);
			add_event([entity_proxy](ZstSessionAdaptor * dlg) {dlg->on_entity_arriving(entity_proxy); });
		}
		else if (strcmp(entity.entity_type(), PLUG_TYPE) == 0) {
			ZstPlug * plug = new ZstPlug(static_cast<ZstPlug&>(entity));
			entity_proxy = plug;
			dynamic_cast<ZstComponent*>(parent)->add_plug(plug);
			add_event([plug](ZstSessionAdaptor * dlg) {dlg->on_plug_arriving(plug); });
		}
		else {
			ZstLog::net(LogLevel::notification, "Can't create unknown proxy entity type {}", entity.entity_type());
		}

		ZstLog::net(LogLevel::notification, "Received proxy entity {}", entity_proxy->URI().path());

		//Set entity as a proxy so the reaper can clean it up later
		synchronisable_set_proxy(entity_proxy);

		//Activate entity and dispatch events
		entity_proxy->add_adaptor(this);
		synchronisable_set_activation_status(entity_proxy, ZstSyncStatus::ACTIVATED);
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
	synchronisable_set_activation_status(performer_proxy, ZstSyncStatus::ACTIVATED);

	if (performer.URI() != m_root->URI()) {
		m_clients[performer_proxy->URI()] = performer_proxy;
		synchronisable_set_activation_status(performer_proxy, ZstSyncStatus::ACTIVATED);
		add_event([performer_proxy](ZstSessionAdaptor * dlg) {dlg->on_performer_arriving(performer_proxy); });
	}
}
