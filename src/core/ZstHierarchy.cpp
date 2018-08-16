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
	entity->add_adaptor(static_cast<ZstSynchronisableAdaptor*>(this), true);
	entity->add_adaptor(static_cast<ZstEntityAdaptor*>(this), true);
	synchronisable_set_activating(entity);
}


void ZstHierarchy::destroy_entity(ZstEntityBase * entity, const ZstTransportSendType & sendtype)
{
	if (!entity) return;
	synchronisable_set_deactivating(entity);
}


void ZstHierarchy::add_performer(const ZstPerformer & performer)
{
	//Copy streamable so we have a local ptr for the performer
	ZstPerformer * performer_proxy = new ZstPerformer(performer);
	assert(performer_proxy);
	ZstLog::net(LogLevel::notification, "Adding new performer {}", performer_proxy->URI().path());

	//Since this is a proxy entity, it should be activated immediately.
	synchronisable_set_activation_status(performer_proxy, ZstSyncStatus::ACTIVATED);
	synchronisable_set_proxy(performer_proxy);

	//Add adaptors to performer so we can clean it up later
	performer_proxy->add_adaptor(static_cast<ZstSynchronisableAdaptor*>(this), true);
	performer_proxy->add_adaptor(static_cast<ZstEntityAdaptor*>(this), true);

	//Store it
	m_clients[performer_proxy->URI()] = performer_proxy;

	//Add entity to lookup
	add_entity_to_lookup(performer_proxy);

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
	ZstEntityBase * entity = NULL;
	try {
		entity = m_entity_lookup[path];
	} catch(std::out_of_range){
	}

	return entity;
}

ZstEntityBase * ZstHierarchy::walk_entity(const ZstURI & path)
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
		result = root->walk_child_by_URI(path);
	}

	return result;
}

ZstMsgKind ZstHierarchy::add_proxy_entity(const ZstEntityBase & entity) {

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

	//Lock the hierarchy
	//std::unique_lock<std::mutex> lock(m_hierarchy_mutex);
	//lock.lock();

	//Create proxies and set parents
	if (strcmp(entity.entity_type(), COMPONENT_TYPE) == 0) {
		entity_proxy = new ZstComponent(static_cast<const ZstComponent&>(entity));
		dynamic_cast<ZstContainer*>(parent)->add_child(entity_proxy);
		m_hierarchy_events.defer([entity_proxy](ZstHierarchyAdaptor * adp) {adp->on_entity_arriving(entity_proxy); });
	}
	else if (strcmp(entity.entity_type(), CONTAINER_TYPE) == 0) {
		entity_proxy = new ZstContainer(static_cast<const ZstContainer&>(entity));
		dynamic_cast<ZstContainer*>(parent)->add_child(entity_proxy);
		m_hierarchy_events.defer([entity_proxy](ZstHierarchyAdaptor * adp) {adp->on_entity_arriving(entity_proxy); });
	}
	else if (strcmp(entity.entity_type(), PLUG_TYPE) == 0) {
		ZstPlug * plug = new ZstPlug(static_cast<const ZstPlug&>(entity));
		entity_proxy = plug;
		dynamic_cast<ZstComponent*>(parent)->add_plug(plug);
		m_hierarchy_events.defer([plug](ZstHierarchyAdaptor * adp) {adp->on_plug_arriving(plug); });
	}
	else {
		ZstLog::net(LogLevel::notification, "Can't create unknown proxy entity type {}", entity.entity_type());
		return ZstMsgKind::ERR_MSG_TYPE_UNKNOWN;
	}
	//lock.unlock();

	//Cache entity in lookup map
	add_entity_to_lookup(entity_proxy);

	ZstLog::net(LogLevel::notification, "Received proxy entity {}", entity_proxy->URI().path());

	//Set entity as a proxy so the reaper can clean it up later
	synchronisable_set_proxy(entity_proxy);

	//Activate entity and dispatch events
	entity_proxy->add_adaptor(static_cast<ZstSynchronisableAdaptor*>(this), true);
	entity_proxy->add_adaptor(static_cast<ZstEntityAdaptor*>(this), true);

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

void ZstHierarchy::add_entity_to_lookup(ZstEntityBase * entity)
{
	ZstEntityBundleUnique entity_bundle = ZstEntityBundleUnique(entity->aquire_child_bundle(), ZstEntityBase::release_child_bundle);
	for (auto c : *entity_bundle) {
		m_entity_lookup[c->URI()] = c;
	}
	m_entity_lookup[entity->URI()] = entity;
}

void ZstHierarchy::remove_entity_from_lookup(ZstEntityBase * entity)
{
	ZstEntityBundleUnique entity_bundle = ZstEntityBundleUnique(entity->aquire_child_bundle(), ZstEntityBase::release_child_bundle);
	for (auto c : *entity_bundle) {
		try {
			m_entity_lookup.erase(c->URI());
		}
		catch (std::out_of_range) {
		}
	}
	try {
		m_entity_lookup.erase(entity->URI());
	}
	catch (std::out_of_range) {
	}
}

void ZstHierarchy::activate_entity_complete(ZstEntityBase * entity)
{
	if (!entity) {
		ZstLog::net(LogLevel::warn, "activate_entity_complete(): Entity not found");
		return;
	}

	//Add entity to lookup tables
	add_entity_to_lookup(entity);

	//Finally, enqueue activation
	synchronisable_enqueue_activation(entity);
}

void ZstHierarchy::destroy_entity_complete(ZstEntityBase * entity)
{
	if (!entity) {
		ZstLog::net(LogLevel::warn, "destroy_entity_complete(): Entity not found");
		return;
	}

	synchronisable_set_destroyed(entity);

	ZstContainer * parent = NULL;

	//Lock the hierarchy
	//std::unique_lock<std::mutex> lock(m_hierarchy_mutex);
	//lock.lock();

	//Remove entity from parent
	if (entity->parent()) {
		parent = dynamic_cast<ZstContainer*>(entity->parent());
		parent->remove_child(entity);
	}
	else {
		//Entity is a root performer. Remove from performer list
		try {
			m_clients.erase(entity->URI());
		}
		catch (std::out_of_range) {
			ZstLog::net(LogLevel::warn, "Could not remove performer {}", entity->URI().path());
		}
	}

	//Pre-emptively clear cables from this entity, they'll be leaving anyway, and this will avoid issues 
	//where a parent entity leaves before a child does and so does not clear its internal cable list
	//ZstCableBundleUnique cable_bundle = ZstCableBundleUnique(entity->aquire_cable_bundle(), ZstEntityBase::release_cable_bundle);
	//for (auto c : *cable_bundle) {
	//	c->disconnect();
	//}

	//Dispatch events depending on entity type
	if (strcmp(entity->entity_type(), PLUG_TYPE) == 0) {
		//Remove plug
		ZstPlug * plug = dynamic_cast<ZstPlug*>(entity);
		parent->remove_plug(plug);

		hierarchy_events().defer([plug](ZstHierarchyAdaptor * dlg) {
			dlg->on_plug_leaving(plug);
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

	//Remove entity from quick lookup map
	remove_entity_from_lookup(entity);

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
