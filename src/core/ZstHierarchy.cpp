#include <showtime/ZstLogging.h>
#include "ZstHierarchy.h"

namespace showtime {

ZstHierarchy::ZstHierarchy() :
	m_hierarchy_events(std::make_shared<ZstEventDispatcher<ZstHierarchyAdaptor> >())
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
	if (entity) {
		if (entity->is_registered()) {
			return;
		}

		Log::entity(Log::Level::notification, "Registering entity {}", entity->URI().path());

		//Add module adaptors to entity
		entity->add_adaptor(ZstHierarchyAdaptor::downcasted_shared_from_this<ZstHierarchyAdaptor>());
		entity->add_adaptor(ZstSynchronisableAdaptor::downcasted_shared_from_this<ZstSynchronisableAdaptor>());
		entity->add_adaptor(ZstEntityAdaptor::downcasted_shared_from_this<ZstEntityAdaptor>());

		//Store entity in lookup table
		add_entity_to_lookup(entity);

		//Set entity registration flag
		entity_set_registered(entity, true);
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

void ZstHierarchy::deactivate_entity(ZstEntityBase * entity, const ZstTransportRequestBehaviour & sendtype)
{
	if (!entity) return;

	if(entity->activation_status() != ZstSyncStatus::DESTROYED)
		synchronisable_set_deactivating(entity);
}

void ZstHierarchy::get_performers(ZstEntityBundle & bundle) const
{
	get_performers(&bundle);
}

void ZstHierarchy::get_performers(ZstEntityBundle* bundle) const
{
	// Add local performer
	auto local_performer = get_local_performer();
	if (local_performer)
		bundle->add(get_local_performer());

	// Only add performers to the bundle
	for (auto&& entity : m_proxies) {
		auto performer = dynamic_cast<ZstPerformer*>(entity.get());
		if (performer)
			bundle->add(performer);
	}
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
		//Log::net(Log::Level::debug, "Couldn't find entity {}", path.path());
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
		Log::net(Log::Level::error, "Can't create entity {}, it already exists", entity_path.path());
		return NULL;
	}

	// All entities need a parent unless they are a performer 
	auto parent_path = entity_path.parent();
	ZstEntityBase* parent = find_entity(parent_path);

	if (entity_type != EntityTypes_Performer) {
		if (!parent) {
			Log::net(Log::Level::error, "Entity {} has no parent", entity_path.path());
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
	entity->get_child_entities(&bundle, true, true);

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
        m_hierarchy_events->defer([entity](ZstHierarchyAdaptor* adaptor) {
			adaptor->on_entity_arriving(entity);
		});
    }
    else if (entity->entity_type() == ZstEntityType::FACTORY) {
        m_hierarchy_events->defer([entity](ZstHierarchyAdaptor* adaptor) {
			adaptor->on_factory_arriving(static_cast<ZstEntityFactory*>(entity));
		});
    }
	else if (entity->entity_type() == ZstEntityType::PERFORMER) {
		//Dispatch events
		m_hierarchy_events->defer([entity](ZstHierarchyAdaptor* adaptor) {
			adaptor->on_performer_arriving(static_cast<ZstPerformer*>(entity));
		});
	}
}

void ZstHierarchy::update_proxy_entity(ZstEntityBase* original, const EntityTypes entity_type, const EntityData* entity_data, const void* payload)
{
	if (!original)
		return;

	auto entity_path = ZstURI(entity_data->URI()->c_str(), entity_data->URI()->size());
	Log::net(Log::Level::notification, "Entity {} received an update", entity_path.path());

	if (entity_type == EntityTypes_Factory) {
		auto local_factory = dynamic_cast<ZstEntityFactory*>(original);
		if (local_factory) {
			auto factory_data = static_cast<const Factory*>(payload);
			local_factory->clear_creatables();
			for (auto c : *factory_data->factory()->creatables()) {
				local_factory->add_creatable(ZstURI(c->c_str(), c->size()), [](const char* name) { return std::unique_ptr<ZstEntityBase>{}; });
			}
			local_factory->update_creatables();
		}
		else {
			Log::net(Log::Level::warn, "Could not find local proxy instance of remote factory {}", entity_path.path());
			return;
		}
	}

	// A rename occured if the original and new paths don't match up
	if (original->URI() != entity_path) {
		if (original->parent_address() != entity_path.parent()) {
			Log::net(Log::Level::warn, "Move operations not implemented yet");
			return;
		}
		original->set_name(entity_path.last().path());

		hierarchy_events()->defer([original](ZstHierarchyAdaptor* adp) {
			adp->on_entity_updated(original);
		});
	}
}

void ZstHierarchy::remove_proxy_entity(ZstEntityBase * entity)
{
	if (entity) {
		if (entity->is_proxy()) {
			Log::net(Log::Level::notification, "Destroying entity {}", entity->URI().path());
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
		Log::net(Log::Level::warn, "Entity {} was not in the entity lookup map");
	}
}

void ZstHierarchy::update_entity_in_lookup(ZstEntityBase* entity, const ZstURI& original_path)
{
	std::lock_guard<std::recursive_mutex> lock(m_hierarchy_mutex);
	try {
		m_entity_lookup.erase(original_path);
	}
	catch (std::out_of_range) {
		Log::net(Log::Level::warn, "Entity {} was not in the entity lookup map");
	}

	if (entity->parent_address().is_empty()) {
		// Entity has no parent - don't add to lookup
	}


	m_entity_lookup[entity->URI()] = entity;
}

void ZstHierarchy::activate_entity_complete(ZstEntityBase * entity)
{
	if (!entity) {
		Log::net(Log::Level::warn, "activate_entity_complete(): Entity not found");
		return;
	}

	//Add entity to lookup tables
	ZstEntityBundle bundle;
	entity->get_child_entities(&bundle, true, true);
	for (auto c : bundle) {
		add_entity_to_lookup(c);
        synchronisable_set_activating(c);
        synchronisable_enqueue_activation(c);
	}
}

void ZstHierarchy::destroy_entity_complete(ZstEntityBase * entity)
{
	if (!entity) {
		Log::net(Log::Level::warn, "destroy_entity_complete(): Entity not found");
		return;
	}

	// Remove child from parent
	auto parent = entity->parent();
	if (parent) {
		parent->remove_child(entity);
	}

	//Dispatch events depending on entity type
	// We only need to dispatch events for proxy entities since we would have initiated
	// entity removal locally otherwise
	if (entity->is_proxy()) {
		if (entity->entity_type() == ZstEntityType::PERFORMER)
		{
			hierarchy_events()->defer([path = entity->URI()](ZstHierarchyAdaptor* adaptor) {
				adaptor->on_performer_leaving(path);
			});
		}
		else if (entity->entity_type() == ZstEntityType::FACTORY)
		{
			hierarchy_events()->defer([path = entity->URI()](ZstHierarchyAdaptor* adaptor) {
				adaptor->on_factory_leaving(path);
			});
		}
		else if (entity->entity_type() == ZstEntityType::COMPONENT)
		{
			hierarchy_events()->defer([path = entity->URI()](ZstHierarchyAdaptor* adaptor) {
				adaptor->on_entity_leaving(path);
			});
		}
	}

	//Cleanup children
	ZstEntityBundle bundle;
	entity->get_child_entities(&bundle, true, true);
	for (auto c : bundle) {
		//Enqueue deactivation
		synchronisable_enqueue_deactivation(c);
	}
}

std::shared_ptr<ZstEventDispatcher<ZstHierarchyAdaptor> > & ZstHierarchy::hierarchy_events()
{
	return m_hierarchy_events;
}

 void ZstHierarchy::process_events()
{
	m_hierarchy_events->process_events();    
    ZstSynchronisableModule::process_events();

	// Update entities on the event loop thread that need to tick
	for (auto entity : m_ticking_entities) {
		std::lock_guard<std::recursive_mutex> lock(m_hierarchy_mutex);
		entity->on_tick();
	}
}

 void ZstHierarchy::flush_events()
 {
    ZstSynchronisableModule::flush_events();
 }

void ZstHierarchy::on_synchronisable_destroyed(ZstSynchronisable * synchronisable, bool already_removed)
{
	//Synchronisable is going away and the stage needs to know
	if (synchronisable->is_activated() || synchronisable->activation_status() == ZstSyncStatus::DESTROYED) {
		auto sendtype = ZstTransportRequestBehaviour::SYNC_REPLY;
		if (already_removed)
			sendtype = ZstTransportRequestBehaviour::PUBLISH;
		deactivate_entity(dynamic_cast<ZstEntityBase*>(synchronisable), sendtype);
	}

	if (already_removed) {
		// Make sure that we don't call this synchronisable object in the future
		add_dead_synchronisable_ID(synchronisable->instance_id());
		reaper_cleanup_entity(dynamic_cast<ZstEntityBase*>(synchronisable)->URI());
		return;
	}

	reaper().add_cleanup_op([
		this, 
		synchronisable_id = synchronisable->instance_id(), 
		path = dynamic_cast<ZstEntityBase*>(synchronisable)->URI(), 
		is_proxy = synchronisable->is_proxy()
	]() {
		Log::net(Log::Level::debug, "on_synchronisable_destroyed cleanup: Path: {} ID: {}", path.path(), synchronisable_id);

		if (already_removed_synchronisable(synchronisable_id))
			return;

		//Remove entity from quick lookup map
		reaper_cleanup_entity(path);

		if (is_proxy) {
			auto it = std::find_if(m_proxies.begin(), m_proxies.end(), [synchronisable_id](const std::unique_ptr<ZstSynchronisable>& item) {
				return(item.get()->instance_id() == synchronisable_id) ? true : false;
			});

			if (it != m_proxies.end()) {
				std::lock_guard<std::recursive_mutex> lock(m_hierarchy_mutex);
				m_proxies.erase(it);
			}
		}
	});
	
	//reaper().add(synchronisable);
	synchronisable_set_destroyed(synchronisable);
}

void ZstHierarchy::reaper_cleanup_entity(const ZstURI& entity) {
	this->remove_entity_from_lookup(entity);
}

void ZstHierarchy::on_register_entity(ZstEntityBase * entity)
{
	//Register entity to stage
	activate_entity(entity);
}

void ZstHierarchy::register_entity_tick(ZstEntityBase* entity)
{
	if (!entity) return;
	if (entity->is_proxy()) {
		Log::net(Log::Level::warn, "Can't tick proxy entity {}", entity->URI().path());
		return;
	}

	std::lock_guard<std::recursive_mutex> lock(m_hierarchy_mutex);
	m_ticking_entities.insert(entity);
}

void ZstHierarchy::unregister_entity_tick(ZstEntityBase* entity)
{
	std::lock_guard<std::recursive_mutex> lock(m_hierarchy_mutex);
	m_ticking_entities.erase(entity);
}

}
