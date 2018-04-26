#include "ZstClientHierarchy.h"
#include <boost/assign.hpp>

ZstClientHierarchy::ZstClientHierarchy() :
	m_root(NULL)
{
}

ZstClientHierarchy::~ZstClientHierarchy()
{
}

void ZstClientHierarchy::destroy()
{
	//TODO: Delete proxies and templates
	delete m_root;
}

void ZstClientHierarchy::init(std::string name)
{
	//Create a root entity to hold our local entity hierarchy
	//Sets the name of our performer and the address of our graph output
	m_root = new ZstPerformer(name.c_str());
	m_root->add_adaptor(this);
}

void ZstClientHierarchy::process_events()
{
	ZstEventDispatcher<ZstSynchronisableAdaptor*>::process_events();
	ZstEventDispatcher<ZstStageDispatchAdaptor*>::process_events();
	ZstEventDispatcher<ZstSessionAdaptor*>::process_events();
}

void ZstClientHierarchy::flush()
{
	ZstEventDispatcher<ZstSynchronisableAdaptor*>::flush();
	ZstEventDispatcher<ZstStageDispatchAdaptor*>::flush();
	ZstEventDispatcher<ZstSessionAdaptor*>::flush();
}

void ZstClientHierarchy::on_receive_from_stage(size_t payload_index, ZstMessage * msg)
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

void ZstClientHierarchy::notify_event_ready(ZstSynchronisable * synchronisable)
{
	add_event([this, synchronisable](ZstSessionAdaptor * dlg) { this->synchronisable_process_events(synchronisable); });
}

void ZstClientHierarchy::synchronise_graph(bool async)
{
	//Ask the stage to send us a full snapshot
	ZstLog::net(LogLevel::notification, "Requesting stage snapshot");
	
	run_event([this, async](ZstStageDispatchAdaptor * adaptor) {
		adaptor->send_message(ZstMsgKind::CLIENT_SYNC, async, [this](ZstMessageReceipt response) {
			this->synchronise_graph_complete(response);
		});
	});
}

void ZstClientHierarchy::synchronise_graph_complete(ZstMessageReceipt response)
{
	synchronisable_enqueue_activation(m_root);
	ZstLog::net(LogLevel::notification, "Graph sync completed");
}

void ZstClientHierarchy::activate_entity(ZstEntityBase * entity, bool async)
{
	//If the entity doesn't have a parent, put it under the root container
	if (!entity->parent()) {
		ZstLog::net(LogLevel::notification, "No parent set for {}, adding to {}", entity->URI().path(), m_root->URI().path());
		m_root->add_child(entity);
	}

	ZstHierarchy::activate_entity(entity, async);
	
	//Build message
	run_event([this, entity, async](ZstStageDispatchAdaptor * adaptor) {
		adaptor->send_entity_message(entity, async, [this, entity](ZstMessageReceipt response) {
			this->activate_entity_complete(response, entity);
		});
	});
}


void ZstClientHierarchy::activate_entity_complete(ZstMessageReceipt response, ZstEntityBase * entity)
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


void ZstClientHierarchy::destroy_entity(ZstEntityBase * entity, bool async)
{
	ZstHierarchy::destroy_entity(entity, async);

	//If the entity is local, let the stage know it's leaving
	if (!entity->is_proxy()) {

		run_event([this, async, entity](ZstStageDispatchAdaptor * adaptor) {
			adaptor->send_message(ZstMsgKind::DESTROY_ENTITY, async, std::string(entity->URI().path()), [this, entity](ZstMessageReceipt response) {
				this->destroy_entity_complete(response, entity);
			});
		});
		
		if (!async) {				
			process_events();
		}
	}
	else {
		destroy_entity_complete(ZstMessageReceipt{ZstMsgKind::OK, async}, entity);
	}
}

void ZstClientHierarchy::destroy_entity_complete(ZstMessageReceipt response, ZstEntityBase * entity)
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


void ZstClientHierarchy::destroy_plug(ZstPlug * plug, bool async)
{
	if (!plug->is_proxy()) {
		run_event([this, async, plug](ZstStageDispatchAdaptor * adaptor) {
			adaptor->send_message(ZstMsgKind::DESTROY_ENTITY, async, std::string(plug->URI().path()), [this, plug](ZstMessageReceipt response) {
				this->destroy_plug_complete(response, plug); 
			});
		});
	}
	else {
		destroy_plug_complete(ZstMessageReceipt{ ZstMsgKind::EMPTY , async}, plug);
	}
}

void ZstClientHierarchy::destroy_plug_complete(ZstMessageReceipt response, ZstPlug * plug)
{
	synchronisable_set_destroyed(plug);

	ZstComponent * parent = dynamic_cast<ZstComponent*>(plug->parent());
	parent->remove_plug(plug);

	//Queue events
	synchronisable_enqueue_deactivation(plug);
	add_event([plug](ZstSessionAdaptor * dlg){ dlg->on_plug_leaving(plug); });
}

ZstEntityBase * ZstClientHierarchy::find_entity(const ZstURI & path)
{
	ZstEntityBase * result = NULL;
	ZstPerformer * root = NULL;
	
	//Performer is local
	if (path_is_local(path)) {
		result = m_root->find_child_by_URI(path);
	}
	else {
		result = ZstHierarchy::find_entity(path);
	}

	return result;
}

bool ZstClientHierarchy::path_is_local(const ZstURI & path) {
	return path.contains(m_root->URI());
}

void ZstClientHierarchy::add_proxy_entity(ZstEntityBase & entity) {

	// Don't need to activate local entities, they will auto-activate when the stage responds with an OK
	// Also, we can't rely on the proxy flag here as it won't have been set yet
	if (path_is_local(entity.URI())) {
		ZstLog::net(LogLevel::notification, "Received local entity {}. Ignoring", entity.URI().path());
		return;
	}

	ZstHierarchy::add_proxy_entity(entity);
}


ZstPerformer * ZstClientHierarchy::get_local_performer() const
{
	return m_root;
}


void ZstClientHierarchy::add_performer(ZstPerformer & performer)
{
	if (performer.URI() == m_root->URI()) {
		ZstLog::net(LogLevel::debug, "Received self {} as performer. Ignoring", m_root->URI().path());
		return;
	}

	ZstHierarchy::add_performer(performer);
}
