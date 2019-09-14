#include <memory>
#include <nlohmann/json.hpp>

#include "entities/ZstComponent.h"
#include "../ZstEventDispatcher.hpp"

namespace showtime
{
    ZstComponent::ZstComponent() :
        ZstEntityBase("")
    {
        set_entity_type(EntityType::EntityType_COMPONENT);
        set_component_type("");
    }

    ZstComponent::ZstComponent(const char * path) :
        ZstEntityBase(path)
    {
        set_entity_type(EntityType::EntityType_COMPONENT);
        set_component_type("");
    }

    ZstComponent::ZstComponent(const char * component_type, const char * path)
        : ZstEntityBase(path)
    {
        set_entity_type(EntityType::EntityType_COMPONENT);
        set_component_type(component_type);
    }

    ZstComponent::ZstComponent(const ZstComponent & other) : ZstEntityBase(other)
    {
        for (auto c : other.m_children) {
            
            if (c.second->entity_type() == EntityType_COMPONENT) {
                add_child(new ZstComponent(*dynamic_cast<ZstComponent*>(c.second)));
            }
            else if(c.second->entity_type() == EntityType_PLUG) {
                ZstPlug * plug = dynamic_cast<ZstPlug*>(c.second);
                if (plug->direction() == PlugDirection_IN_JACK) {
                    add_child(new ZstInputPlug(*dynamic_cast<ZstInputPlug*>(plug)));
                } else if (plug->direction() == PlugDirection_OUT_JACK){
                    add_child(new ZstOutputPlug(*dynamic_cast<ZstOutputPlug*>(plug)));
                }
            }
        }
        
        m_component_type = other.m_component_type;
    }

    ZstComponent::~ZstComponent()
    {
        ZstEntityBundle bundle;
        get_child_entities(bundle, false);
        for (auto child : bundle) {
            // We only delete proxy entities, since they are owned by this library
            if(child->is_proxy())
                delete child;
        }
        m_children.clear();
    }

    ZstInputPlug * ZstComponent::create_input_plug(const char * name, ValueType val_type)
    {
        return create_input_plug(name, val_type, -1);
    }

    ZstInputPlug * ZstComponent::create_input_plug(const char * name, ValueType val_type, int max_cable_connections)
    {
        ZstInputPlug * plug = new ZstInputPlug(name, val_type, max_cable_connections);
        add_child(plug);
        return plug;
    }

    ZstOutputPlug * ZstComponent::create_output_plug(const char * name, ValueType val_type, bool reliable)
    {
        ZstOutputPlug * plug = new ZstOutputPlug(name, val_type, reliable);
        add_child(plug);
        return plug;
    }

    ZstEntityBundle & ZstComponent::get_plugs(ZstEntityBundle & bundle) const
    {
        for(auto entity : m_children){
            if(entity.second->entity_type() ==EntityType_PLUG)
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
            entity_events()->invoke([entity](std::shared_ptr<ZstEntityAdaptor> adaptor) {
                adaptor->on_request_entity_activation(entity);
            });
        }
    }

    void ZstComponent::remove_child(ZstEntityBase * entity)
    {
        if (!entity || is_destroyed())
            return;
        
        //Remove cables associated with this child
        ZstCableBundle bundle;
        entity->get_child_cables(bundle);
        for (auto cable : bundle){
            entity_events()->defer([cable](std::shared_ptr<ZstEntityAdaptor> adaptor){
                adaptor->on_disconnect_cable(cable);
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
        get_child_entities(bundle, false);
        for (auto child : bundle) {
            this->remove_child(child);
            this->add_child(child);
        }
    }

    void ZstComponent::serialize(flatbuffers::Offset<Component> & serialized_offset, flatbuffers::FlatBufferBuilder & buffer_builder) const
    {
        auto component_builder = ComponentBuilder(buffer_builder);
        auto component_type_offset = buffer_builder.CreateString(m_component_type.c_str(), m_component_type.size());
        component_builder.add_component_type(component_type_offset);
        
        flatbuffers::Offset<Entity> entity;
        ZstEntityBase::serialize(entity, buffer_builder);
        component_builder.add_entity(entity);
        
        serialized_offset = component_builder.Finish();
    }

    void ZstComponent::deserialize(const Component* buffer)
    {
        m_component_type = buffer->component_type()->str();
        ZstEntityBase::deserialize(buffer->entity());
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

    void ZstComponent::get_child_cables(ZstCableBundle & bundle)
    {
        ZstEntityBundle entity_bundle;
        get_child_entities(entity_bundle, false);
        for(auto child : entity_bundle){
            child->get_child_cables(bundle);
        }
        
        ZstEntityBase::get_child_cables(bundle);
    }

    void ZstComponent::get_child_entities(ZstEntityBundle & bundle, bool include_parent)
    {
        for (auto child : m_children) {
            child.second->get_child_entities(bundle);
        }
        ZstEntityBase::get_child_entities(bundle, include_parent);
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
}
