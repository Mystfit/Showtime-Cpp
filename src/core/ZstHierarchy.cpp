#include "ZstLogging.h"
#include "ZstHierarchy.h"

ZstHierarchy::ZstHierarchy() :
    m_hierarchy_events("hierarchy")
{
}

ZstHierarchy::~ZstHierarchy()
{
}

void ZstHierarchy::destroy()
{
    ZstEntityBundle bundle;
    get_performers(bundle);
    for(auto performer : bundle){
        delete performer;
    }
    m_clients.clear();
    
	m_hierarchy_events.remove_all_adaptors();
    ZstSynchronisableModule::destroy();
}

void ZstHierarchy::activate_entity(ZstEntityBase * entity, const ZstTransportSendType & sendtype)
{
	//If this is not a local entity, we can't activate it
	if (entity->is_proxy())
		return;

	//Add module adaptors to entity and children
	ZstEntityBundle bundle;
    entity->get_child_entities(bundle, true);
	for (auto c : bundle) {
		synchronisable_set_activating(c);
		c->add_adaptor(static_cast<ZstSynchronisableAdaptor*>(this));
		c->add_adaptor(static_cast<ZstEntityAdaptor*>(this));
	}
}

ZstEntityBase * ZstHierarchy::create_entity(const ZstURI & creatable_path, const char * name, const ZstTransportSendType & sendtype)
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
		//ZstLog::net(LogLevel::debug, "Couldn't find entity {}", path.path());
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

ZstMsgKind ZstHierarchy::add_proxy_entity(const ZstEntityBase & entity)
{
	// All entities need a parent unless they are a performer 
	ZstURI parent_URI = entity.URI().parent();
	if (!parent_URI.size()) {
		ZstLog::net(LogLevel::error, "Entity {} has no parent", entity.URI().path());
		return ZstMsgKind::ERR_ENTITY_NOT_FOUND;
	}

    //Check if the entity already exists in the hierarchy
	if (find_entity(entity.URI())) {
		ZstLog::net(LogLevel::error, "Can't create entity {}, it already exists", entity.URI().path());
		return ZstMsgKind::ERR_ENTITY_ALREADY_EXISTS;
	}
    
    ZstEntityBase * parent = find_entity(parent_URI);
	if (!parent) {
		ZstLog::net(LogLevel::error, "Could not find parent {} for entity {}", parent_URI.path(), entity.URI().path());
		return ZstMsgKind::ERR_ENTITY_NOT_FOUND;
	}

    ZstEntityBase * entity_proxy = NULL;

	if (strcmp(entity.entity_type(), COMPONENT_TYPE) == 0) {
		entity_proxy = new ZstComponent(static_cast<const ZstComponent&>(entity));
	}
	else if (strcmp(entity.entity_type(), PLUG_TYPE) == 0) {
		entity_proxy = new ZstPlug(static_cast<const ZstPlug&>(entity));
	}
	else if (strcmp(entity.entity_type(), FACTORY_TYPE) == 0) {
		entity_proxy = new ZstEntityFactory(static_cast<const ZstEntityFactory&>(entity));
	}
	else {
		ZstLog::net(LogLevel::notification, "Can't create unknown proxy entity type {}", entity.entity_type());
		return ZstMsgKind::ERR_MSG_TYPE_UNKNOWN;
	}
    
    //Set the entity as a proxy early to avoid accidental auto-activation
    synchronisable_set_proxy(entity_proxy);
    dynamic_cast<ZstComponent*>(parent)->add_child(entity_proxy);
    
	//Mirror proxy and adaptor addition to entity children
	ZstEntityBundle bundle;
    entity_proxy->get_child_entities(bundle, true);
    for (auto c : bundle)
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

	return ZstMsgKind::OK;
}

void ZstHierarchy::dispatch_entity_arrived_event(ZstEntityBase * entity){
    if(!entity)
        return;
    
    //Only dispatch events once all entities have been activated and registered
    if (strcmp(entity->entity_type(), COMPONENT_TYPE) == 0 || strcmp(entity->entity_type(), PLUG_TYPE) == 0)  {
        m_hierarchy_events.defer([entity](ZstHierarchyAdaptor * adp) {adp->on_entity_arriving(entity); });
    }
    else if (strcmp(entity->entity_type(), FACTORY_TYPE) == 0) {
        m_hierarchy_events.defer([entity](ZstHierarchyAdaptor * adp) {adp->on_factory_arriving(static_cast<ZstEntityFactory*>(entity)); });
    }
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
    entity->get_child_entities(bundle);
	for (auto c : bundle) {
		add_entity_to_lookup(c);
        synchronisable_set_activating(c);
        synchronisable_enqueue_activation(c);
	}
}

void ZstHierarchy::destroy_entity_complete(ZstEntityBase * entity)
{
	if (!entity) {
		ZstLog::net(LogLevel::warn, "destroy_entity_complete(): Entity not found");
		return;
	}

	//Remove entity from parent
	if (entity->parent()) {
        ZstComponent * parent = dynamic_cast<ZstComponent*>(entity->parent());
        if(parent)
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

	//Cleanup children
	ZstEntityBundle bundle;
    entity->get_child_entities(bundle, true);
	for (auto c : bundle) {
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
    
    ZstSynchronisableModule::process_events();
}

 void ZstHierarchy::flush_events()
 {
	m_hierarchy_events.flush();
     
    ZstSynchronisableModule::flush_events();
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
