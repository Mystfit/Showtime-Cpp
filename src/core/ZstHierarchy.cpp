#include "ZstLogging.h"
#include "ZstHierarchy.h"

namespace showtime {

ZstHierarchy::ZstHierarchy() :
	m_hierarchy_events(std::make_shared<ZstEventDispatcher< std::shared_ptr<ZstHierarchyAdaptor> > >("hierarchy_events"))
{
}

ZstHierarchy::~ZstHierarchy()
{
    ZstEntityBundle bundle;
    get_performers(bundle);
    for(auto performer : bundle){
        //TODO: Leaking performers
        //delete performer;
    }
    m_clients.clear();
}

void ZstHierarchy::destroy()
{
}

void ZstHierarchy::activate_entity(ZstEntityBase * entity, const ZstTransportRequestBehaviour & sendtype)
{
	//If this is not a local entity, we can't activate it
	if (entity->is_proxy())
		return;

	//Add module adaptors to entity and children
	ZstEntityBundle bundle;
    entity->get_child_entities(bundle, true);
	for (auto c : bundle) {
		synchronisable_set_activating(c);//ZstSynchronisableAdaptor
		c->add_adaptor(ZstSynchronisableAdaptor::downcasted_shared_from_this<ZstSynchronisableAdaptor>());
        
        auto entity_adp = ZstEntityAdaptor::downcasted_shared_from_this<ZstEntityAdaptor>();
		c->add_adaptor(entity_adp);
	}
}

ZstEntityBase * ZstHierarchy::create_entity(const ZstURI & creatable_path, const char * name, const ZstTransportRequestBehaviour & sendtype)
{
	ZstEntityBase * entity = NULL;
	
	ZstEntityFactory * factory = dynamic_cast<ZstEntityFactory*>(find_entity(creatable_path.parent()));
	if (factory) {
		entity = factory->create_entity(creatable_path, name);
	}

	return entity;
}

void ZstHierarchy::destroy_entity(ZstEntityBase * entity, const ZstTransportRequestBehaviour & sendtype)
{
	if (!entity) return;
	if(entity->activation_status() != ZstSyncStatus::DESTROYED)
		synchronisable_set_deactivating(entity);
}

void ZstHierarchy::add_performer(const Entity* performer)
{
	//Copy streamable so we have a local ptr for the performer
	ZstPerformer * performer_proxy = new ZstPerformer(performer);
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
		c->add_adaptor(ZstSynchronisableAdaptor::downcasted_shared_from_this<ZstSynchronisableAdaptor>());
        
        auto entity_adp = ZstEntityAdaptor::downcasted_shared_from_this<ZstEntityAdaptor>();
		c->add_adaptor(entity_adp);
	}

	//Store performer
	m_clients[performer_proxy->URI()] = performer_proxy;

	//Dispatch events
	m_hierarchy_events->defer([performer_proxy](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
		adaptor->on_performer_arriving(performer_proxy);
	});
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

void ZstHierarchy::add_proxy_entity(const Entity* entity)
{
    // Check if the entity already exists in the hierarchy
    if (find_entity(ZstURI(entity->URI()->c_str()))) {
        ZstLog::net(LogLevel::error, "Can't create entity {}, it already exists", entity->URI()->str());
        return;
    }
    
	// All entities need a parent unless they are a performer 
	auto parent_URI = ZstURI(entity->URI()->c_str(), entity->URI()->size()).parent();
	if (!parent_URI.size()) {
		ZstLog::net(LogLevel::error, "Entity {} has no parent", entity->URI()->str());
		return;
	}
    
    ZstEntityBase * parent = find_entity(parent_URI);
	if (!parent) {
		ZstLog::net(LogLevel::error, "Could not find parent {} for entity {}", parent_URI.path(), entity->URI()->str());
		return;
	}
    
    // Create the entity proxy
    auto entity_proxy = unpack_entity(entity);
    
    // Set the entity as a proxy early to avoid accidental auto-activation
    synchronisable_set_proxy(entity_proxy.get());
    dynamic_cast<ZstComponent*>(parent)->add_child(entity_proxy.get());
    
	// Mirror proxy and adaptor addition to entity children
	ZstEntityBundle bundle;
    entity_proxy->get_child_entities(bundle, true);
    for (auto c : bundle)
	{
		//Set entity as a proxy so the reaper can clean it up later
		synchronisable_set_proxy(c);

		//Cache entity in lookup map
		add_entity_to_lookup(c);

		//Add adaptors to entities
		c->add_adaptor(ZstSynchronisableAdaptor::downcasted_shared_from_this<ZstSynchronisableAdaptor>());
        
        auto entity_adp = ZstEntityAdaptor::downcasted_shared_from_this<ZstEntityAdaptor>();
		c->add_adaptor(entity_adp);

		//Activate entity
		synchronisable_set_activation_status(c, ZstSyncStatus::ACTIVATED);
	}
}

void ZstHierarchy::dispatch_entity_arrived_event(ZstEntityBase * entity){
    if(!entity)
        return;
    
    //Only dispatch events once all entities have been activated and registered
    if (entity->entity_type() == EntityType_COMPONENT || entity->entity_type() == EntityType_PLUG)  {
        m_hierarchy_events->defer([entity](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->on_entity_arriving(entity);
		});
    }
    else if (entity->entity_type() == EntityType_FACTORY) {
        m_hierarchy_events->defer([entity](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->on_factory_arriving(static_cast<ZstEntityFactory*>(entity));
		});
    }
}

void ZstHierarchy::update_proxy_entity(const Entity* entity)
{
    // TODO: Make this work with ANY entity type that wants to update itself.
    
	if (entity->entity_type() == EntityType_FACTORY) {
		ZstLog::net(LogLevel::notification, "Factory {} received an update", entity->URI()->str());

		ZstEntityFactory remote_factory = ZstEntityFactory(entity);
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
			ZstLog::net(LogLevel::warn, "Could not find local proxy instance of remote factory {}", entity->URI()->str());
			return;
		}
	}
}

void ZstHierarchy::remove_proxy_entity(ZstEntityBase * entity)
{
	if (entity) {
		if (entity->is_proxy()) {
			ZstLog::net(LogLevel::notification, "Destroying entity {}", entity->URI().path());
			destroy_entity_complete(entity);
		}
	}
}
    
std::shared_ptr<ZstEntityBase> ZstHierarchy::unpack_entity(const Entity* entity_buffer)
{
    switch(entity_buffer->entity_type()){
        case EntityType_PERFORMER:
            return std::make_unique<ZstPerformer>(entity_buffer);
        case EntityType_COMPONENT:
            return std::make_unique<ZstComponent>(entity_buffer);
        case EntityType_PLUG:
            if(entity_buffer->plug_direction() == PlugDirection_IN_JACK)
                return std::make_unique<ZstInputPlug>(entity_buffer);
            return std::make_unique<ZstOutputPlug>(entity_buffer);
        case EntityType_FACTORY:
            return std::make_unique<ZstEntityFactory>(entity_buffer);
        case EntityType_UNKNOWN:
            throw std::runtime_error("Can't parse unknown entity {}");
    }
}

void ZstHierarchy::add_entity_to_lookup(ZstEntityBase * entity)
{
	m_entity_lookup[entity->URI()] = entity;
}

void ZstHierarchy::remove_entity_from_lookup(const ZstURI & entity)
{
	try {
		m_entity_lookup.erase(entity);
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
		remove_entity_from_lookup(c->URI());
	}

	//Dispatch events depending on entity type
	if (entity->entity_type() == EntityType_PLUG) {
		hierarchy_events()->defer([entity](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->on_plug_leaving(static_cast<ZstPlug*>(entity));
		});
	}
	else if (entity->entity_type() == EntityType_PERFORMER)
	{
		hierarchy_events()->defer([entity](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->on_performer_leaving(static_cast<ZstPerformer*>(entity));
		});
	}
	else if (entity->entity_type() == EntityType_FACTORY)
	{
		hierarchy_events()->defer([entity](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->on_factory_leaving(static_cast<ZstEntityFactory*>(entity));
		});
	}
	else
	{
		hierarchy_events()->defer([entity](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->on_entity_leaving(entity);
		});
	}
}

std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstHierarchyAdaptor> > > & ZstHierarchy::hierarchy_events()
{
	return m_hierarchy_events;
}

 void ZstHierarchy::process_events()
{
	m_hierarchy_events->process_events();    
    ZstSynchronisableModule::process_events();
}

 void ZstHierarchy::flush_events()
 {
	m_hierarchy_events->flush();
    ZstSynchronisableModule::flush_events();
 }

void ZstHierarchy::on_synchronisable_destroyed(ZstSynchronisable * synchronisable)
{
	//Synchronisable is going away and the stage needs to know
	if (synchronisable->is_activated() || synchronisable->activation_status() == ZstSyncStatus::DESTROYED) {
		destroy_entity(dynamic_cast<ZstEntityBase*>(synchronisable), ZstTransportRequestBehaviour::PUBLISH);
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

}
