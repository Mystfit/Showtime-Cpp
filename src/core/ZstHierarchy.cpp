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
	m_proxies.clear();
}

void ZstHierarchy::activate_entity(ZstEntityBase * entity, const ZstTransportRequestBehaviour & sendtype)
{
	register_entity(entity);
}

void ZstHierarchy::init_adaptors()
{
	//Add self as an adaptor for processing deferred events
	hierarchy_events()->add_adaptor(ZstHierarchyAdaptor::downcasted_shared_from_this<ZstHierarchyAdaptor>());
	ZstSynchronisableModule::init_adaptors();
}

void ZstHierarchy::register_entity(ZstEntityBase* entity)
{
	//Add module adaptors to entity
	entity->add_adaptor(ZstHierarchyAdaptor::downcasted_shared_from_this<ZstHierarchyAdaptor>());
	entity->add_adaptor(ZstSynchronisableAdaptor::downcasted_shared_from_this<ZstSynchronisableAdaptor>());
	entity->add_adaptor(ZstEntityAdaptor::downcasted_shared_from_this<ZstEntityAdaptor>());

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

void ZstHierarchy::deactivate_entity(ZstEntityBase * entity, const ZstTransportRequestBehaviour & sendtype)
{
	if (!entity) return;
	if(entity->activation_status() != ZstSyncStatus::DESTROYED)
		synchronisable_set_deactivating(entity);
}

ZstEntityBundle & ZstHierarchy::get_performers(ZstEntityBundle & bundle) const
{
	// Add local performer
	auto local_performer = get_local_performer();
	if(local_performer)
		bundle.add(get_local_performer());

	// Only add performers to the bundle
	for (auto&& entity : m_proxies) {
		auto performer = dynamic_cast<ZstPerformer*>(entity.get());
		if (performer)
			bundle.add(performer);
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
	ZstPerformer* root = dynamic_cast<ZstPerformer*>(find_entity(path.first()));

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

std::unique_ptr<ZstEntityBase> ZstHierarchy::create_proxy_entity(EntityTypes entity_type, const EntityData* entity_data, const void* payload)
{
	// Check if the entity already exists in the hierarchy
	auto entity_path = ZstURI(entity_data->URI()->c_str(), entity_data->URI()->size());
	if (find_entity(entity_path)) {
		ZstLog::net(LogLevel::error, "Can't create entity {}, it already exists", entity_path.path());
		return NULL;
	}

	// All entities need a parent unless they are a performer 
	auto parent_path = entity_path.parent();
	ZstEntityBase* parent = NULL;

	if (entity_type != EntityTypes_Performer) {
		if (parent_path.is_empty()) {
			ZstLog::net(LogLevel::error, "Entity {} has no parent", entity_path.path());
			return NULL;
		}
	}

	// Create proxy
	return unpack_entity(entity_type, payload);
}

void ZstHierarchy::add_proxy_entity(std::unique_ptr<ZstEntityBase> entity)
{
	if (!entity)
		return;

	auto parent = find_entity(entity->URI().parent());

    // Set the entity as a proxy early to avoid accidental auto-activation
    synchronisable_set_proxy(entity.get());

	// Add the child to its parent (if it has one)
	if(parent)
		dynamic_cast<ZstComponent*>(parent)->add_child(entity.get());

	//Register entity adaptors
	register_entity(entity.get());
    
	// Propagate proxy properties to children of this proxy
	ZstEntityBundle bundle;
	entity->get_child_entities(bundle, true, true);
	if (entity->entity_type() == ZstEntityType::PERFORMER) {
		dynamic_cast<ZstPerformer*>(entity.get())->get_factories(bundle);
	}

    for (auto c : bundle){
		//Set entity as a proxy so the reaper can clean it up later
		synchronisable_set_proxy(c);
		synchronisable_set_activation_status(c, ZstSyncStatus::ACTIVATED);
	}

	auto entity_ptr = entity.get();
	
	// Move entity into hierarchy to manage its lifetime
	m_proxies.insert(std::move(entity));

	//Dispatch entity arrived event regardless if the entity is local or remote
	dispatch_entity_arrived_event(entity_ptr);
}

void ZstHierarchy::dispatch_entity_arrived_event(ZstEntityBase * entity){
    if(!entity)
        return;
    
    //Only dispatch events once all entities have been activated and registered
    if (entity->entity_type() == ZstEntityType::COMPONENT || entity->entity_type() == ZstEntityType::PLUG)  {
        m_hierarchy_events->defer([entity](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->on_entity_arriving(entity);
		});
    }
    else if (entity->entity_type() == ZstEntityType::FACTORY) {
        m_hierarchy_events->defer([entity](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->on_factory_arriving(static_cast<ZstEntityFactory*>(entity));
		});
    }
	else if (entity->entity_type() == ZstEntityType::PERFORMER) {
		//Dispatch events
		m_hierarchy_events->defer([entity](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->on_performer_arriving(static_cast<ZstPerformer*>(entity));
		});
	}
}

void ZstHierarchy::update_proxy_entity(const EntityTypes entity_type, const EntityData* entity_data, const void* payload)
{
    // TODO: Make this work with ANY entity type that wants to update itself.
	//throw(std::runtime_error("Not implemented"));

	if (entity_type == EntityTypes_Factory) {
		auto entity_path = ZstURI(entity_data->URI()->c_str(), entity_data->URI()->size());
		auto factory_data = static_cast<const Factory*>(payload);

		ZstLog::net(LogLevel::notification, "Factory {} received an update", entity_path.path());

		ZstEntityFactory * local_factory = dynamic_cast<ZstEntityFactory*>(find_entity(entity_path));
		if (local_factory) {
			local_factory->clear_creatables();
			for (auto c : *factory_data->factory()->creatables()) {
				local_factory->add_creatable(ZstURI(c->c_str(), c->size()));
			}
			local_factory->update_creatables();
		}
		else {
			ZstLog::net(LogLevel::warn, "Could not find local proxy instance of remote factory {}", entity_path.path());
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
	return NULL;
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
	entity->get_child_entities(bundle, true, true);
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
	if (entity->entity_type() == ZstEntityType::PLUG) {
		hierarchy_events()->defer([entity](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->on_plug_leaving(static_cast<ZstPlug*>(entity));
			});
	}
	else if (entity->entity_type() == ZstEntityType::PERFORMER)
	{
		hierarchy_events()->defer([entity](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->on_performer_leaving(static_cast<ZstPerformer*>(entity));
			});
	}
	else if (entity->entity_type() == ZstEntityType::FACTORY)
	{
		hierarchy_events()->defer([entity](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->on_factory_leaving(static_cast<ZstEntityFactory*>(entity));
			});
	}
	else if (entity->entity_type() == ZstEntityType::COMPONENT)
	{
		hierarchy_events()->defer([entity](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			adaptor->on_entity_leaving(entity);
		});
	}

	//Remove entity from parent
	if (!entity->parent_address().is_empty()) {
		auto parent = entity->parent();
		if (parent) {
			ZstComponent* parent_c = dynamic_cast<ZstComponent*>(parent);
			if (parent_c)
				parent_c->remove_child(entity);
		}
	}

	//Cleanup children
	ZstEntityBundle bundle;
	entity->get_child_entities(bundle, true, true);
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
		deactivate_entity(dynamic_cast<ZstEntityBase*>(synchronisable), ZstTransportRequestBehaviour::PUBLISH);
	}

	if (already_removed) {
		remove_entity_from_lookup((dynamic_cast<ZstEntityBase*>(synchronisable)) ? dynamic_cast<ZstEntityBase*>(synchronisable)->URI() : ZstURI());
		return;
	}

    reaper().add_cleanup_op([this, synchronisable]() {
		//Remove entity from quick lookup map
		std::lock_guard<std::recursive_mutex> lock(m_hierarchy_mutex);
		
		auto entity = dynamic_cast<ZstEntityBase*>(synchronisable);
		if (entity) {
			auto p = entity->parent();
			if (p) {
				p->remove_child(entity->parent());
			}

			remove_entity_from_lookup(entity->URI());
		}

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
