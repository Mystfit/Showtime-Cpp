#include "ZstClientHierarchy.h"
#include <boost/assign.hpp>
#include "ZstMessageDispatcher.h"

ZstClientHierarchy::ZstClientHierarchy() :
	m_root(NULL),
	m_stage_events("hierarchy stage"),
	m_synchronisable_events("hierarchy syncronisables with events")
{
}

ZstClientHierarchy::~ZstClientHierarchy()
{
}

void ZstClientHierarchy::destroy()
{
	ZstHierarchy::destroy();
	
	//TODO: Delete other clients
	delete m_root;
}

void ZstClientHierarchy::init(std::string name)
{
	//Create a root entity to hold our local entity hierarchy
	//Sets the name of our performer and the address of our graph output
	m_root = new ZstPerformer(name.c_str());
	m_root->add_adaptor(this);

	//We add this instance as an adaptor to make sure we can process local queued events
	m_synchronisable_events.add_adaptor(this);
}

void ZstClientHierarchy::process_events()
{
	ZstHierarchy::process_events();
	m_synchronisable_events.process_events();
	m_stage_events.process_events();
}

void ZstClientHierarchy::flush_events()
{
	ZstHierarchy::flush_events();
	m_synchronisable_events.flush();
	m_stage_events.flush();
}

void ZstClientHierarchy::on_receive_from_stage(ZstStageMessage * msg)
{
	//Ignore messages with no payloads
	if(msg->num_payloads() < 1){
		return;
	}

	switch (msg->kind()) {
	case ZstMsgKind::CREATE_PLUG:
	{
		ZstPlug plug = msg->unpack_payload_serialisable<ZstPlug>(0);
		add_proxy_entity(plug);
		break;
	}
	case ZstMsgKind::CREATE_PERFORMER:
	{
		ZstPerformer performer = msg->unpack_payload_serialisable<ZstPerformer>(0);
		add_performer(performer);
		break;
	}
	case ZstMsgKind::CREATE_COMPONENT:
	{
		ZstComponent component = msg->unpack_payload_serialisable<ZstComponent>(0);
		add_proxy_entity(component);
		break;
	}
	case ZstMsgKind::CREATE_CONTAINER:
	{
		ZstContainer container = msg->unpack_payload_serialisable<ZstContainer>(0);
		add_proxy_entity(container);
		break;
	}
	case ZstMsgKind::DESTROY_ENTITY:
	{
		remove_proxy_entity(find_entity(ZstURI((char*)msg->payload_at(0).data(), msg->payload_at(0).size())));
		break;
	}
	default:
		break;
	}
}

void ZstClientHierarchy::synchronisable_has_event(ZstSynchronisable * synchronisable)
{
	m_synchronisable_events.defer([this, synchronisable](ZstSynchronisableAdaptor * dlg) {
		this->synchronisable_process_events(synchronisable); 
	});
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
	m_stage_events.invoke([this, entity, async](ZstStageDispatchAdaptor * adaptor) {
		adaptor->send_entity_message(entity, async, [this, entity](ZstMessageReceipt response) {
			this->activate_entity_complete(response, entity);
		});
	});

	if (!async)
		process_events();
}


void ZstClientHierarchy::activate_entity_complete(ZstMessageReceipt response, ZstEntityBase * entity)
{
	if (response.status != ZstMsgKind::OK) {
		ZstLog::net(LogLevel::error, "Activate entity {} failed with status {}", entity->URI().path(), ZstMsgNames[response.status]);
		return;
	}

	switch (response.status) {
	case ZstMsgKind::OK:
		synchronisable_enqueue_activation(entity);
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
		m_stage_events.invoke([this, async, entity](ZstStageDispatchAdaptor * adaptor) {
			adaptor->send_message(ZstMsgKind::DESTROY_ENTITY, async, std::string(entity->URI().path()), [this, entity](ZstMessageReceipt response) {
				this->destroy_entity_complete(response, entity);
				entity->remove_adaptor(this);
			});
		});
	}
	else {
		destroy_entity_complete(ZstMessageReceipt{ZstMsgKind::OK, async}, entity);
	}

	if (!async) {
		process_events();
		entity->remove_adaptor(this);
	}
}

void ZstClientHierarchy::destroy_entity_complete(ZstMessageReceipt response, ZstEntityBase * entity)
{
	if (!entity) return;

	synchronisable_set_destroyed(entity);

	if (response.status != ZstMsgKind::OK) {
		ZstLog::net(LogLevel::notification, "Destroy entity failed with status {}", ZstMsgNames[response.status]);
	}
	ZstContainer * parent = NULL;

	//Remove entity from parent
	if (entity->parent()) {
		parent = dynamic_cast<ZstContainer*>(entity->parent());
		parent->remove_child(entity);
	}
	else {
		//Entity is a root performer. Remove from performer list
		m_clients.erase(entity->URI());
	}

	//Pre-emptively disconnect all cables inside the entity
	ZstCableBundle * bundle = entity->acquire_cable_bundle();
	bundle->disconnect_all();
	entity->release_cable_bundle(bundle);
	
	//Finally, add non-local entities to the reaper to destroy them at the correct time
	//TODO: Only destroying proxy entities at the moment. Local entities should be managed by the host application
	
	if (strcmp(entity->entity_type(), PLUG_TYPE) == 0) {
		parent->remove_plug(dynamic_cast<ZstPlug*>(entity));
		events().defer([entity](ZstHierarchyAdaptor * dlg) { dlg->on_plug_leaving(static_cast<ZstPlug*>(entity)); });
	}
	else {
		events().defer([entity](ZstHierarchyAdaptor * dlg) {dlg->on_entity_leaving(entity); });
	}
	synchronisable_enqueue_deactivation(entity);
}


ZstEntityBase * ZstClientHierarchy::find_entity(const ZstURI & path)
{
	if (path_is_local(path)) {
		//Path points to local performer
		if (m_root->URI() == path) {
			return m_root;
		}

		//Path points to child in local performer
		return m_root->find_child_by_URI(path);
	}
	else {
		//Path is somewhere else in the hierarchy
		//TODO: Should the local performer be also placed in the remote performer list too?
		return ZstHierarchy::find_entity(path);
	}

	return NULL;
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

ZstEventDispatcher<ZstStageDispatchAdaptor*>& ZstClientHierarchy::stage_events()
{
	return m_stage_events;
}

void ZstClientHierarchy::add_performer(ZstPerformer & performer)
{
	if (performer.URI() == m_root->URI()) {
		ZstLog::net(LogLevel::debug, "Received self {} as performer. Ignoring", m_root->URI().path());
		return;
	}

	ZstHierarchy::add_performer(performer);
}
