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

	//Add module adaptors to entity and children
	ZstEntityBundle bundle;
	for (auto c : entity->get_child_entities(bundle, true)) {
		synchronisable_set_activating(c);
		c->add_adaptor(static_cast<ZstSynchronisableAdaptor*>(this));
		c->add_adaptor(static_cast<ZstEntityAdaptor*>(this));
	}
}

ZstEntityBase * ZstHierarchy::create_entity(const ZstURI & creatable_path, const char * name, bool activate, const ZstTransportSendType & sendtype)
{
	ZstEntityBase * entity = NULL;
	
	ZstEntityFactory * factory = dynamic_cast<ZstEntityFactory*>(find_entity(creatable_path.parent()));
	if (factory) {
		entity = factory->create_entity(creatable_path, name);
	}

	return entity;
}

void ZstHierarchy::destroy_entity(ZstEntityBase * entity, const ZstTransportSendType & sendtype)
{
	if (!entity) return;
	if(entity->activation_status() != ZstSyncStatus::DESTROYED)
		synchronisable_set_deactivating(entity);
}

void ZstHierarchy::add_performer(const ZstPerformer & performer)
{
	//Copy streamable so we have a local ptr for the performer
	ZstPerformer * performer_proxy = new ZstPerformer(performer);
	assert(performer_proxy);
	ZstLog::net(LogLevel::notification, "Adding new performer {}", performer_proxy->URI().path());

	//Populate bundle with factory and child entities
	ZstEntityBundle bundle;
	performer_proxy->get_factories(bundle);
	performer_proxy->get_child_entities(bundle, true);

	//Activate all entities in bundle
	for (auto c : bundle) {
		//Since this is a proxy entity, it should be activated immediately.
		synchronisable_set_activation_status(c, ZstSyncStatus::ACTIVATED);
		synchronisable_set_proxy(c);
		add_entity_to_lookup(c);

		//Add adaptors to performer so we can clean it up later
		c->add_adaptor(static_cast<ZstSynchronisableAdaptor*>(this));
		c->add_adaptor(static_cast<ZstEntityAdaptor*>(this));
	}

	//Store performer
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

ZstEntityBundle & ZstHierarchy::get_performers(ZstEntityBundle & bundle) const
{
	for (auto performer : m_clients) {
		bundle.add(performer.second);
	}
	return bundle;
}

ZstEntityBase * ZstHierarchy::find_entity(const ZstURI & path) const
{
	ZstEntityBase * entity = NULL;

	try {
		entity = m_entity_lookup.at(path);
	} catch(std::out_of_range){
		ZstLog::net(LogLevel::debug, "Couldn't find entity {}", path.path());
	}

	return entity;
}

ZstEntityBase * ZstHierarchy::walk_to_entity(const ZstURI & path) const
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
	}
	else if (strcmp(entity.entity_type(), CONTAINER_TYPE) == 0) {
		entity_proxy = new ZstContainer(static_cast<const ZstContainer&>(entity));
		dynamic_cast<ZstContainer*>(parent)->add_child(entity_proxy);
	}
	else if (strcmp(entity.entity_type(), PLUG_TYPE) == 0) {
		ZstPlug * plug = new ZstPlug(static_cast<const ZstPlug&>(entity));
		entity_proxy = plug;
		dynamic_cast<ZstComponent*>(parent)->add_plug(plug);
	}
	else if (strcmp(entity.entity_type(), FACTORY_TYPE) == 0) {
		ZstEntityFactory * factory = new ZstEntityFactory(static_cast<const ZstEntityFactory&>(entity));
		entity_proxy = factory;
		ZstPerformer * performer = dynamic_cast<ZstPerformer*>(parent);
		if (performer) {
			performer->add_child(factory);
		}
		else {
			ZstLog::net(LogLevel::error, "No parent performer found for factory or parent {} is not a performer", parent->URI().path());
		}
	}
	else {
		ZstLog::net(LogLevel::notification, "Can't create unknown proxy entity type {}", entity.entity_type());
		return ZstMsgKind::ERR_MSG_TYPE_UNKNOWN;
	}
	//lock.unlock();

	ZstLog::net(LogLevel::notification, "Received proxy entity {}", entity_proxy->URI().path());

	//Mirror proxy and adaptor addition to entity children
	ZstEntityBundle bundle;
	for (auto c : entity_proxy->get_child_entities(bundle, true)) 
	{
		//Set entity as a proxy so the reaper can clean it up later
		synchronisable_set_proxy(c);

		//Cache entity in lookup map
		add_entity_to_lookup(c);

		//Add adaptors to entities
		c->add_adaptor(static_cast<ZstSynchronisableAdaptor*>(this));
		c->add_adaptor(static_cast<ZstEntityAdaptor*>(this));

		//Activate entity
		synchronisable_set_activation_status(c, ZstSyncStatus::ACTIVATED);
	}

	//Only dispatch events once all entities have been activated and registered
	if (strcmp(entity.entity_type(), COMPONENT_TYPE) == 0) {
		m_hierarchy_events.defer([entity_proxy](ZstHierarchyAdaptor * adp) {adp->on_entity_arriving(entity_proxy); });
	}
	else if (strcmp(entity.entity_type(), CONTAINER_TYPE) == 0) {
		m_hierarchy_events.defer([entity_proxy](ZstHierarchyAdaptor * adp) {adp->on_entity_arriving(entity_proxy); });
	}
	else if (strcmp(entity.entity_type(), PLUG_TYPE) == 0) {
		m_hierarchy_events.defer([entity_proxy](ZstHierarchyAdaptor * adp) {adp->on_plug_arriving(static_cast<ZstPlug*>(entity_proxy)); });
	}
	else if (strcmp(entity.entity_type(), FACTORY_TYPE) == 0) {
		m_hierarchy_events.defer([entity_proxy](ZstHierarchyAdaptor * adp) {adp->on_factory_arriving(static_cast<ZstEntityFactory*>(entity_proxy)); });
	}

	return ZstMsgKind::OK;
}

ZstMsgKind ZstHierarchy::update_proxy_entity(const ZstEntityBase & entity)
{
	if (strcmp(entity.entity_type(), FACTORY_TYPE) == 0) {
		ZstLog::net(LogLevel::notification, "Factory {} received an update", entity.URI().path());

		ZstEntityFactory remote_factory = static_cast<const ZstEntityFactory&>(entity);
		ZstEntityFactory * local_factory = dynamic_cast<ZstEntityFactory*>(find_entity(remote_factory.URI()));
		if (local_factory) {
			local_factory->clear_creatables();
			ZstURIBundle updated_creatables;
			for (auto c : remote_factory.get_creatables(updated_creatables)) {
				local_factory->add_creatable(c);
			}
			local_factory->update_creatables();
		}
		else {
			return ZstMsgKind::ERR_ENTITY_NOT_FOUND;
		}
	}
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
	m_entity_lookup[entity->URI()] = entity;
}

void ZstHierarchy::remove_entity_from_lookup(ZstEntityBase * entity)
{
	try {
		m_entity_lookup.erase(entity->URI());
	}
	catch (std::out_of_range) {
		ZstLog::net(LogLevel::warn, "Entity {} was not in the entity lookup map");
	}
}

void ZstHierarchy::activate_entity_complete(ZstEntityBase * entity)
{
	if (!entity) {
		ZstLog::net(LogLevel::warn, "activate_entity_complete(): Entity not found");
		return;
	}

	//Add entity to lookup tables
	ZstEntityBundle bundle;
	for (auto c : entity->get_child_entities(bundle, false)) {
		add_entity_to_lookup(c);
		synchronisable_set_activated(c);
	}

	//Add entity to the lookup seperately since it was not included in the bundle so that we can activate it seperately
	add_entity_to_lookup(entity);
	synchronisable_enqueue_activation(entity);
}

void ZstHierarchy::destroy_entity_complete(ZstEntityBase * entity)
{
	if (!entity) {
		ZstLog::net(LogLevel::warn, "destroy_entity_complete(): Entity not found");
		return;
	}

	ZstContainer * parent = NULL;

	//Lock the hierarchy
	//std::unique_lock<std::mutex> lock(m_hierarchy_mutex);
	//lock.lock();

	//Remove entity from parent
	if (entity->parent()) {
		if (strcmp(entity->entity_type(), PLUG_TYPE) == 0) {
			//Remove plug
			ZstPlug * plug = dynamic_cast<ZstPlug*>(entity);
			parent->remove_plug(plug);
		}
		else {
			parent = dynamic_cast<ZstContainer*>(entity->parent());
			parent->remove_child(entity);
		}
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

	//Cleanup children
	ZstEntityBundle bundle;
	for (auto c : entity->get_child_entities(bundle, true)) {
		//Enqueue deactivation
		synchronisable_enqueue_deactivation(c);
		
		//Remove entity from quick lookup map
		remove_entity_from_lookup(c);
	}

	//Dispatch events depending on entity type
	if (strcmp(entity->entity_type(), PLUG_TYPE) == 0) {
		hierarchy_events().defer([entity](ZstHierarchyAdaptor * dlg) {
			dlg->on_plug_leaving(static_cast<ZstPlug*>(entity));
		});
	}
	else if (strcmp(entity->entity_type(), PERFORMER_TYPE) == 0)
	{
		hierarchy_events().defer([entity](ZstHierarchyAdaptor * adp) {
			adp->on_performer_leaving(static_cast<ZstPerformer*>(entity));
		});
	}
	else if (strcmp(entity->entity_type(), FACTORY_TYPE) == 0)
	{
		hierarchy_events().defer([entity](ZstHierarchyAdaptor * adp) {
			adp->on_factory_leaving(static_cast<ZstEntityFactory*>(entity));
		});
	}
	else
	{
		hierarchy_events().defer([entity](ZstHierarchyAdaptor * dlg) {
			dlg->on_entity_leaving(entity);
		});
	}
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

void ZstHierarchy::on_synchronisable_has_event(ZstSynchronisable * synchronisable)
{
	m_synchronisable_events.defer([this, synchronisable](ZstSynchronisableAdaptor * dlg) {
		this->synchronisable_process_events(synchronisable);
	});
}

void ZstHierarchy::on_synchronisable_destroyed(ZstSynchronisable * synchronisable)
{
	//Synchronisable is going away and the stage needs to know
	if (synchronisable->is_activated() || synchronisable->activation_status() == ZstSyncStatus::DESTROYED) {
		destroy_entity(dynamic_cast<ZstEntityBase*>(synchronisable), ZstTransportSendType::PUBLISH);
	}

	if (synchronisable->is_proxy())
		reaper().add(synchronisable);

	synchronisable_set_destroyed(synchronisable);
}

void ZstHierarchy::on_register_entity(ZstEntityBase * entity)
{
	//Register entity to stage
	activate_entity(entity);
}
