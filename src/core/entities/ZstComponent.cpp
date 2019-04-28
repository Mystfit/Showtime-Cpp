#include <memory>
#include <nlohmann/json.hpp>

#include "entities/ZstComponent.h"
#include "../ZstEventDispatcher.hpp"

ZstComponent::ZstComponent() : 
	ZstEntityBase("")
{
	set_entity_type(COMPONENT_TYPE);
	set_component_type("");
}

ZstComponent::ZstComponent(const char * path) :
	ZstEntityBase(path)
{
	set_entity_type(COMPONENT_TYPE);
	set_component_type("");
}

ZstComponent::ZstComponent(const char * component_type, const char * path)
    : ZstEntityBase(path)
{
	set_entity_type(COMPONENT_TYPE);
	set_component_type(component_type);
}

ZstComponent::ZstComponent(const ZstComponent & other) : ZstEntityBase(other)
{
    for (auto c : other.m_children) {
        
        if (strcmp(c.second->entity_type(), COMPONENT_TYPE) == 0) {
            add_child(new ZstComponent(*dynamic_cast<ZstComponent*>(c.second)));
        }
        else if (strcmp(c.second->entity_type(), PLUG_TYPE) == 0) {
            ZstPlug * plug = dynamic_cast<ZstPlug*>(c.second);
            if (plug->direction() == ZstPlugDirection::IN_JACK) {
                add_child(new ZstInputPlug(*dynamic_cast<ZstInputPlug*>(plug)));
            } else if (plug->direction() == ZstPlugDirection::OUT_JACK){
                add_child(new ZstOutputPlug(*dynamic_cast<ZstOutputPlug*>(plug)));
            }
        }
    }
	
	m_component_type = other.m_component_type;
}

ZstComponent::~ZstComponent()
{
    if (!is_proxy()) {
        ZstEntityBundle bundle;
        for (auto child : get_child_entities(bundle, false)) {
            // TODO: Deleting children will crash if the host GC's them after we delete them here
            //ZstLog::entity(LogLevel::debug, "FIXME: Component {} leaking entity {} to avoid host app crashing when GCing", URI().path(), child.second->URI().path());
            //delete child;
        }
        m_children.clear();
    }
}

ZstInputPlug * ZstComponent::create_input_plug(const char * name, ZstValueType val_type)
{
	return create_input_plug(name, val_type, -1);
}

ZstInputPlug * ZstComponent::create_input_plug(const char * name, ZstValueType val_type, int max_cable_connections)
{
	ZstInputPlug * plug = new ZstInputPlug(name, val_type, max_cable_connections);
	add_child(plug);
	return plug;
}

ZstOutputPlug * ZstComponent::create_output_plug(const char * name, ZstValueType val_type, bool reliable)
{
	ZstOutputPlug * plug = new ZstOutputPlug(name, val_type, reliable);
	add_child(plug);
	return plug;
}

ZstEntityBundle & ZstComponent::get_plugs(ZstEntityBundle & bundle) const
{
    for(auto entity : m_children){
        if(strcmp(entity.second->entity_type(), PLUG_TYPE) == 0)
            bundle.add(entity.second);
    }
    return bundle;
}

void ZstComponent::add_child(ZstEntityBase * entity, bool auto_activate)
{
    if (is_destroyed()) return;

    if(m_children.find(entity->URI()) != m_children.end()){
        ZstLog::entity(LogLevel::warn, "Entity {} is already a child of {}", entity->URI().path(), this->URI().path());
        return;
    }
    
    //Super add child handles parenting effects inside the child
    ZstEntityBase::add_child(entity);
    
    //Store the entity in our child list
    m_children[entity->URI()] = entity;
    
    if (is_activated() && !entity->is_proxy() && auto_activate) {
        entity_events()->invoke([entity](ZstEntityAdaptor * adp) { adp->on_request_entity_activation(entity); });
    }
}

void ZstComponent::remove_child(ZstEntityBase * entity)
{
	if (!entity || is_destroyed())
		return;
    
    //Remove cables associated with this child
	ZstCableBundle bundle;
	for (auto cable : entity->get_child_cables(bundle)){
        entity_events()->defer([cable](ZstEntityAdaptor * adp){
            adp->on_disconnect_cable(cable);
        });
	}
    
    //Clear child from maps
    auto c = m_children.find(entity->URI());
    if (c != m_children.end()) {
        m_children.erase(c);
    }
    
    ZstEntityBase::remove_child(entity);
}

void ZstComponent::set_parent(ZstEntityBase * parent)
{
    if(is_destroyed()) return;

    ZstEntityBase::set_parent(parent);
    
    ZstEntityBundle bundle;
    for (auto child : get_child_entities(bundle, false)) {
        this->remove_child(child);
        this->add_child(child);
    }
}

void ZstComponent::write_json(json & buffer) const
{
	//Pack container
	ZstEntityBase::write_json(buffer);

	//Pack component type
	buffer["component_type"] = component_type();

    //Pack children
    buffer["children"] = json::array();
    for(auto child : m_children){
        buffer["children"].push_back(child.second->as_json());
    }
}

void ZstComponent::read_json(const json & buffer)
{
	//Unpack entity base first
	ZstEntityBase::read_json(buffer);

	//Unpack component type
    set_component_type(buffer["component_type"].get<std::string>().c_str(),  buffer["component_type"].get<std::string>().size());

    //Unpack children
    for (auto c : buffer["children"]) {
        ZstEntityBase * child = NULL;
        
        if (c["entity_type"] == COMPONENT_TYPE) {
            child = new ZstComponent();
        } else if(c["entity_type"] == PLUG_TYPE){
            if (c["plug_direction"] == ZstPlugDirection::IN_JACK) {
                child = new ZstInputPlug();
            }
            else if (c["plug_direction"] == ZstPlugDirection::OUT_JACK){
                child = new ZstOutputPlug();
            } else{
                // ????
            }
        } else {
            std::string entity_t = c.at("entity_type");
            ZstLog::entity(LogLevel::warn, "Child is a {}", entity_t);
        }
        if(child){
            child->read_json(c);
            add_child(child);
        }
    }
}

const char * ZstComponent::component_type() const
{
	return m_component_type.c_str();
}

void ZstComponent::set_component_type(const char * component_type)
{
	set_component_type(component_type, strlen(component_type));
}

void ZstComponent::set_component_type(const char * component_type, size_t len)
{
	m_component_type = std::string(component_type, len);
}

ZstCableBundle & ZstComponent::get_child_cables(ZstCableBundle & bundle)
{
    ZstEntityBundle entity_bundle;
    for(auto child : get_child_entities(entity_bundle, false)){
        child->get_child_cables(bundle);
    }
	return ZstEntityBase::get_child_cables(bundle);
}

ZstEntityBundle & ZstComponent::get_child_entities(ZstEntityBundle & bundle, bool include_parent)
{
    for (auto child : m_children) {
        child.second->get_child_entities(bundle);
    }
    return ZstEntityBase::get_child_entities(bundle, include_parent);
}



//--------------

ZstEntityBase * ZstComponent::walk_child_by_URI(const ZstURI & path)
{
    ZstEntityBase * result = NULL;
    ZstEntityBase * previous = NULL;
    
    if (this->URI().size() >= path.size() || !path.contains(URI())) {
        return result;
    }
    
    int distance = static_cast<int>(path.size()) - static_cast<int>(this->URI().size());
    if (distance < 0) assert(distance >= 0);
    
    while(distance > 0) {
        ZstURI next = path.range(0, path.size() - distance);
        result = NULL;
        
        if (!previous) {
            previous = this;
        }
        
        //Check if the parent is a container
        ZstComponent * prev_container = dynamic_cast<ZstComponent*>(previous);
        if (prev_container) {
            result = prev_container->get_child_by_URI(next);
        }
        
        if (result) {
            distance = static_cast<int>(path.size()) - static_cast<int>(result->URI().size());
            previous = result;
        } else {
            break;
        }
    }
    
    return result;
}

ZstEntityBase * ZstComponent::get_child_by_URI(const ZstURI & path)
{
    ZstEntityBase * result = NULL;
    ZstEntityMap::iterator it = m_children.find(path);
    
    if (it != m_children.end())
        result = it->second;
    
    return result;
}

ZstEntityBase * ZstComponent::get_child_at(size_t index) const
{
    ZstEntityBase * result = NULL;
    int i = 0;
    for (auto it : m_children) {
        if (i == index) {
            result = it.second;
            break;
        }
        i++;
    }
    return result;
}

const size_t ZstComponent::num_children() const
{
    return m_children.size();
}
