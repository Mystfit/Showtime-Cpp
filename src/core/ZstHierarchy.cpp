#include <ZstLogging.h>
#include "ZstHierarchy.h"

ZstHierarchy::ZstHierarchy() :
	m_synchronisable_events("hierarchy stage"),
    m_hierarchy_events("hierarchy")
{

}

ZstHierarchy::~ZstHierarchy()
{
}

void ZstHierarchy::init()
{
	//We add this instance as an adaptor to make sure we can process local queued events
	m_synchronisable_events.add_adaptor(this);
}

void ZstHierarchy::destroy()
{
	this->flush_events();
	m_synchronisable_events.remove_all_adaptors();
	m_hierarchy_events.remove_all_adaptors();
}

void ZstHierarchy::activate_entity(ZstEntityBase * entity, const ZstTransportSendType & sendtype)
{
	//If this is not a local entity, we can't activate it
	if (entity->is_proxy())
		return;

	//Register client to entity to allow it to send messages
	entity->add_adaptor_to_children(this);
	synchronisable_set_activating(entity);
}


void ZstHierarchy::destroy_entity(ZstEntityBase * entity, const ZstTransportSendType & sendtype)
{
	if (!entity) return;
	synchronisable_set_deactivating(entity);
}


void ZstHierarchy::add_performer(ZstPerformer & performer)
{
	//Copy streamable so we have a local ptr for the performer
	ZstPerformer * performer_proxy = new ZstPerformer(performer);
	assert(performer_proxy);
	ZstLog::net(LogLevel::notification, "Adding new performer {}", performer_proxy->URI().path());

	//Since this is a proxy entity, it should be activated immediately.
	synchronisable_set_activation_status(performer_proxy, ZstSyncStatus::ACTIVATED);
	synchronisable_set_proxy(performer_proxy);

	//Add adaptors to performer so we can clean it up later
	ZstSynchronisable::add_adaptor(performer_proxy, this);
	
	//Store it
	m_clients[performer_proxy->URI()] = performer_proxy;

	//Dispatch events
	m_hierarchy_events.defer([performer_proxy](ZstHierarchyAdaptor * adp) {adp->on_performer_arriving(performer_proxy); });
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

std::vector<ZstPerformer*> ZstHierarchy::get_performers()
{
	std::vector<ZstPerformer*> performers;
	for (auto performer : m_clients) {
		performers.push_back(performer.second);
	}
	return performers;
}


ZstEntityBase * ZstHierarchy::find_entity(const ZstURI & path)
{
	ZstEntityBase * result = NULL;
	ZstPerformer * root = get_performer_by_URI(path);

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


ZstMsgKind ZstHierarchy::add_proxy_entity(ZstEntityBase & entity) {

	ZstEntityBase * entity_proxy = NULL;

	// All entities need a parent unless they are a performer 
	ZstURI parent_URI = entity.URI().parent();
	if (!parent_URI.size()) {
		ZstLog::net(LogLevel::error, "Entity {} has no parent", entity.URI().path());
		return ZstMsgKind::ERR_ENTITY_NOT_FOUND;
	}

	ZstEntityBase * parent = find_entity(parent_URI);
	assert(parent);

	if (find_entity(entity.URI())) {
		ZstLog::net(LogLevel::error, "Can't create entity {}, it already exists", entity.URI().path());
		return ZstMsgKind::ERR_ENTITY_ALREADY_EXISTS;
	}

	//Create proxies and set parents
	if (strcmp(entity.entity_type(), COMPONENT_TYPE) == 0) {
		entity_proxy = new ZstComponent(static_cast<ZstComponent&>(entity));
		dynamic_cast<ZstContainer*>(parent)->add_child(entity_proxy);
		m_hierarchy_events.defer([entity_proxy](ZstHierarchyAdaptor * adp) {adp->on_entity_arriving(entity_proxy); });
	}
	else if (strcmp(entity.entity_type(), CONTAINER_TYPE) == 0) {
		entity_proxy = new ZstContainer(static_cast<ZstContainer&>(entity));
		dynamic_cast<ZstContainer*>(parent)->add_child(entity_proxy);
		m_hierarchy_events.defer([entity_proxy](ZstHierarchyAdaptor * adp) {adp->on_entity_arriving(entity_proxy); });
	}
	else if (strcmp(entity.entity_type(), PLUG_TYPE) == 0) {
		ZstPlug * plug = new ZstPlug(static_cast<ZstPlug&>(entity));
		entity_proxy = plug;
		dynamic_cast<ZstComponent*>(parent)->add_plug(plug);
		m_hierarchy_events.defer([plug](ZstHierarchyAdaptor * adp) {adp->on_plug_arriving(plug); });
	}
	else {
		ZstLog::net(LogLevel::notification, "Can't create unknown proxy entity type {}", entity.entity_type());
		return ZstMsgKind::ERR_MSG_TYPE_UNKNOWN;
	}

	ZstLog::net(LogLevel::notification, "Received proxy entity {}", entity_proxy->URI().path());

	//Set entity as a proxy so the reaper can clean it up later
	synchronisable_set_proxy(entity_proxy);

	//Activate entity and dispatch events
    ZstSynchronisable::add_adaptor(entity_proxy, this);
	synchronisable_set_activation_status(entity_proxy, ZstSyncStatus::ACTIVATED);
	
	return ZstMsgKind::OK;
}

ZstMsgKind ZstHierarchy::remove_proxy_entity(ZstEntityBase * entity)
{
	if (entity) {
		if (entity->is_proxy()) {
			ZstLog::net(LogLevel::notification, "Destroying entity {}", entity->URI().path());
			destroy_entity_complete(entity);
		}
	}

	return ZstMsgKind::OK;
}

void ZstHierarchy::destroy_entity_complete(ZstEntityBase * entity)
{
	if (!entity) return;

	synchronisable_set_destroyed(entity);

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

	//Dispatch events depending on entity type
	if (strcmp(entity->entity_type(), PLUG_TYPE) == 0) {
		//Remove plug
		parent->remove_plug(dynamic_cast<ZstPlug*>(entity));
		hierarchy_events().defer([entity](ZstHierarchyAdaptor * dlg) {
			dlg->on_plug_leaving(static_cast<ZstPlug*>(entity));
		});
	}
	else if (strcmp(entity->entity_type(), PERFORMER_TYPE) == 0)
	{
		//Remove performer
		hierarchy_events().defer([entity](ZstHierarchyAdaptor * adp) {
			adp->on_performer_leaving(static_cast<ZstPerformer*>(entity));
		});
	}
	else
	{
		//Remove entity
		hierarchy_events().defer([entity](ZstHierarchyAdaptor * dlg) {
			dlg->on_entity_leaving(entity);
		});
	}

	//Finally, add non-local entities to the reaper to destroy them at the correct time
	//TODO: Only destroying proxy entities at the moment. Local entities should be managed by the host application
	synchronisable_enqueue_deactivation(entity);
}

ZstEventDispatcher<ZstHierarchyAdaptor*> & ZstHierarchy::hierarchy_events()
{
	return m_hierarchy_events;
}

 void ZstHierarchy::process_events()
{
	m_synchronisable_events.process_events();
	m_hierarchy_events.process_events();
}

 void ZstHierarchy::flush_events()
 {
	m_synchronisable_events.flush();
	m_hierarchy_events.flush();
 }

void ZstHierarchy::synchronisable_has_event(ZstSynchronisable * synchronisable)
{
	m_synchronisable_events.defer([this, synchronisable](ZstSynchronisableAdaptor * dlg) {
		this->synchronisable_process_events(synchronisable); 
	});
}

void ZstHierarchy::on_synchronisable_destroyed(ZstSynchronisable * synchronisable)
{
	if (synchronisable->is_proxy())
		reaper().add(synchronisable);
}
