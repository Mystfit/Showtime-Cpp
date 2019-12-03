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
	m_clients.clear();
	m_proxies.clear();
    //m_clients.clear();
}

void ZstHierarchy::destroy()
{
}

void ZstHierarchy::activate_entity(ZstEntityBase * entity, const ZstTransportRequestBehaviour & sendtype)
{
	////If this is not a local entity, we can't activate it
	//if (entity->is_proxy() || 
	//	entity->is_activated() || 
	//	entity->activation_status() == ZstSyncStatus::ACTIVATING)
	//	return;
	register_entity(entity);
}

void ZstHierarchy::register_entity(ZstEntityBase* entity)
{
	//Add module adaptors to entity
	entity->add_adaptor(ZstHierarchyAdaptor::downcasted_shared_from_this<ZstHierarchyAdaptor>());
	entity->add_adaptor(ZstSynchronisableAdaptor::downcasted_shared_from_this<ZstSynchronisableAdaptor>());
	entity->add_adaptor(ZstEntityAdaptor::downcasted_shared_from_this<ZstEntityAdaptor>());

	//ZstEntityBundle bundle;
	//entity->get_child_entities(bundle, true);
	//for (auto c : bundle) {
	//	synchronisable_set_activating(c);//ZstSynchronisableAdaptor
	//	c->add_adaptor(ZstHierarchyAdaptor::downcasted_shared_from_this<ZstHierarchyAdaptor>());
	//	c->add_adaptor(ZstSynchronisableAdaptor::downcasted_shared_from_this<ZstSynchronisableAdaptor>());
	//	c->add_adaptor(ZstEntityAdaptor::downcasted_shared_from_this<ZstEntityAdaptor>());
	//}

	//Store entity in lookup table
	add_entity_to_lookup(entity);

	//Set entity registration flag
	entity_set_registered(entity, true);
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

void ZstHierarchy::add_performer(const Performer* performer)
{
	auto performer_proxy = std::make_unique<ZstPerformer>(performer);
	add_performer(performer_proxy.get());
	
	//Store performer
	auto performer_path = ZstURI(performer_proxy->URI());
	m_clients[performer_path] = std::move(performer_proxy);
}

void ZstHierarchy::add_performer(ZstPerformer* performer) {
	ZstLog::net(LogLevel::notification, "Adding new performer {}", performer->URI().path());

	//Register adaptors
	register_entity(performer);

	//Populate bundle with factory and child entities
	ZstEntityBundle bundle;
	performer->get_factories(bundle);
	performer->get_child_entities(bundle, true);

	//Activate all entities in bundle
	for (auto c : bundle) {
		//Since this is a proxy entity, it should be activated immediately.
		synchronisable_set_activation_status(c, ZstSyncStatus::ACTIVATED);
		synchronisable_set_proxy(c);
	}

	//Dispatch events
	m_hierarchy_events->defer([performer](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
		adaptor->on_performer_arriving(performer);
	});
}

ZstPerformer * ZstHierarchy::get_performer_by_URI(const ZstURI & uri) const
{
	ZstPerformer * result = NULL;
	ZstURI performer_URI = uri.first();

	auto entity_iter = m_clients.find(performer_URI);
	if (entity_iter != m_clients.end()) {
		result = entity_iter->second.get();
	}

	return result;
}

ZstEntityBundle & ZstHierarchy::get_performers(ZstEntityBundle & bundle) const
{
	for (auto&& performer : m_clients) {
		bundle.add(performer.second.get());
	}
	return bundle;
}

void ZstHierarchy::update_entity_URI(ZstEntityBase* entity, const ZstURI& original_path)
{
	update_entity_in_lookup(entity, original_path);
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

void ZstHierarchy::add_proxy_entity(const EntityTypes entity_type, const EntityData* entity_data, const void* payload)
{
    // Check if the entity already exists in the hierarchy
	auto entity_path = ZstURI(entity_data->URI()->c_str(), entity_data->URI()->size());
    if (find_entity(entity_path)) {
        ZstLog::net(LogLevel::error, "Can't create entity {}, it already exists", entity_path.path());
        return;
    }
    
	// All entities need a parent unless they are a performer 
	auto parent_path = entity_path.parent();
	if (!parent_path.size()) {
		ZstLog::net(LogLevel::error, "Entity {} has no parent", entity_path.path());
		return;
	}
    
    ZstEntityBase * parent = find_entity(parent_path);
	if (!parent) {
		ZstLog::net(LogLevel::error, "Could not find parent {} for entity {}", parent_path.path(), entity_path.path());
		return;
	}
    
	// Create proxy
	auto entity_proxy = unpack_entity(entity_type, payload);

    // Set the entity as a proxy early to avoid accidental auto-activation
    synchronisable_set_proxy(entity_proxy.get());
    dynamic_cast<ZstComponent*>(parent)->add_child(entity_proxy.get());

	//Register entity adaptors
	register_entity(entity_proxy.get());
    
	// Propagate proxy properties to children of this proxy
	ZstEntityBundle bundle;
    entity_proxy->get_child_entities(bundle, true);
    for (auto c : bundle){
		//Set entity as a proxy so the reaper can clean it up later
		synchronisable_set_proxy(c);
		synchronisable_set_activation_status(c, ZstSyncStatus::ACTIVATED);
	}
	
	// Move entity into hierarchy to manage its lifetime
	m_proxies.insert(std::move(entity_proxy));
}

void ZstHierarchy::dispatch_entity_arrived_event(ZstEntityBase * entity){
    if(!entity)
        return;
    
    //Only dispatch events once all entities have been activated and registered
    if (entity->entity_type() == EntityTypes_Component || entity->entity_type() == EntityTypes_Plug)  {
        m_hierarchy_events->defer([entity](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->on_entity_arriving(entity);
		});
    }
    else if (entity->entity_type() == EntityTypes_Factory) {
        m_hierarchy_events->defer([entity](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->on_factory_arriving(static_cast<ZstEntityFactory*>(entity));
		});
    }
}

void ZstHierarchy::update_proxy_entity(const EntityTypes entity_type, const EntityData* entity_data, const void* payload)
{
    // TODO: Make this work with ANY entity type that wants to update itself.
	throw(std::runtime_error("Not implemented"));



	/*if (entity->entity_type() == EntityTypes_Factory) {
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
	}*/
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
    
ZST_EXPORT const EntityData* ZstHierarchy::get_entity_field(EntityTypes entity_type, const void* data)
{
	switch (entity_type) {
	case EntityTypes_Performer:
		return static_cast<const Performer*>(data)->entity();
	case EntityTypes_Component:
		return static_cast<const Component*>(data)->entity();
	case EntityTypes_Plug:
		return static_cast<const Plug*>(data)->entity();
	case EntityTypes_Factory:
		return static_cast<const Performer*>(data)->entity();
	case EntityTypes_NONE:
		throw std::runtime_error("Can't parse unknown entity {}");
	}
}

std::unique_ptr<ZstEntityBase> ZstHierarchy::unpack_entity(EntityTypes entity_type, const void* entity_data)
{
    switch(entity_type){
        case EntityTypes_Performer:
            return std::make_unique<ZstPerformer>(static_cast<const Performer*>(entity_data));
        case EntityTypes_Component:
            return std::make_unique<ZstComponent>(static_cast<const Component*>(entity_data));
        case EntityTypes_Plug:
            if(static_cast<const Plug*>(entity_data)->plug()->plug_direction() == PlugDirection_IN_JACK)
                return std::make_unique<ZstInputPlug>(static_cast<const Plug*>(entity_data));
            return std::make_unique<ZstOutputPlug>(static_cast<const Plug*>(entity_data));
        case EntityTypes_Factory:
            return std::make_unique<ZstEntityFactory>(static_cast<const Factory*>(entity_data));
        case EntityTypes_NONE:
            throw std::runtime_error("Can't parse unknown entity {}");
    }
	return NULL;
}

void ZstHierarchy::add_entity_to_lookup(ZstEntityBase * entity)
{
	std::lock_guard<std::recursive_mutex> lock(m_hierarchy_mutex);
	m_entity_lookup[entity->URI()] = entity;
}

void ZstHierarchy::remove_entity_from_lookup(const ZstURI & entity)
{
	std::lock_guard<std::recursive_mutex> lock(m_hierarchy_mutex);
	try {
		m_entity_lookup.erase(entity);
	}
	catch (std::out_of_range) {
		ZstLog::net(LogLevel::warn, "Entity {} was not in the entity lookup map");
	}
}

void ZstHierarchy::update_entity_in_lookup(ZstEntityBase* entity, const ZstURI& original_path)
{
	std::lock_guard<std::recursive_mutex> lock(m_hierarchy_mutex);
	try {
		m_entity_lookup.erase(original_path);
	}
	catch (std::out_of_range) {
		ZstLog::net(LogLevel::warn, "Entity {} was not in the entity lookup map");
	}
	m_entity_lookup[entity->URI()] = entity;
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

	//Dispatch events depending on entity type
	if (entity->entity_type() == EntityTypes_Plug) {
		hierarchy_events()->defer([entity](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->on_plug_leaving(static_cast<ZstPlug*>(entity));
			});
	}
	else if (entity->entity_type() == EntityTypes_Performer)
	{
		hierarchy_events()->defer([entity](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->on_performer_leaving(static_cast<ZstPerformer*>(entity));
			});
	}
	else if (entity->entity_type() == EntityTypes_Factory)
	{
		hierarchy_events()->defer([entity](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->on_factory_leaving(static_cast<ZstEntityFactory*>(entity));
			});
	}
	else if (entity->entity_type() == EntityTypes_Component)
	{
		hierarchy_events()->defer([entity](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->on_entity_leaving(entity);
		});
	}

	//Remove entity from parent
	if (entity->parent()) {
		ZstComponent* parent = dynamic_cast<ZstComponent*>(entity->parent());
		if (parent)
			parent->remove_child(entity);
	}

	//Cleanup children
	ZstEntityBundle bundle;
	entity->get_child_entities(bundle, true);
	for (auto c : bundle) {
		//Enqueue deactivation
		synchronisable_enqueue_deactivation(c);
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

void ZstHierarchy::on_synchronisable_destroyed(ZstSynchronisable * synchronisable, bool already_removed)
{
	//Synchronisable is going away and the stage needs to know
	if (synchronisable->is_activated() || synchronisable->activation_status() == ZstSyncStatus::DESTROYED) {
		destroy_entity(dynamic_cast<ZstEntityBase*>(synchronisable), ZstTransportRequestBehaviour::PUBLISH);
	}

	if (already_removed) {
		remove_entity_from_lookup((dynamic_cast<ZstEntityBase*>(synchronisable)) ? dynamic_cast<ZstEntityBase*>(synchronisable)->URI() : ZstURI());
		return;
	}

	reaper().add_cleanup_op([this, already_removed, synchronisable]() {
		//Remove entity from quick lookup map
		std::lock_guard<std::recursive_mutex> lock(m_hierarchy_mutex);
		
		auto entity = dynamic_cast<ZstEntityBase*>(synchronisable);
		if(entity)
			remove_entity_from_lookup(entity->URI());

		if (synchronisable->is_proxy()) {
			auto it = std::find_if(m_proxies.begin(), m_proxies.end(), [synchronisable](const std::unique_ptr<ZstSynchronisable>& item) {
				return(item.get() == synchronisable) ? true : false;
			});

			if (it != m_proxies.end())
				m_proxies.erase(it);
		}
	});
	
	//reaper().add(synchronisable);
	synchronisable_set_destroyed(synchronisable);
}

void ZstHierarchy::on_register_entity(ZstEntityBase * entity)
{
	//Register entity to stage
	activate_entity(entity);
}

}
