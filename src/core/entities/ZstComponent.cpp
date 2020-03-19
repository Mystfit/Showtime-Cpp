#include <memory>

#include "entities/ZstComponent.h"
#include "../ZstEventDispatcher.hpp"
#include "../ZstHierarchy.h"

using namespace flatbuffers;

namespace showtime
{
    ZstComponent::ZstComponent() :
        ZstEntityBase()
    {
        set_entity_type(ZstEntityType::COMPONENT);
        set_component_type("");
    }

    ZstComponent::ZstComponent(const char * path) :
        ZstEntityBase(path)
    {
        set_entity_type(ZstEntityType::COMPONENT);
        set_component_type("");
    }

    ZstComponent::ZstComponent(const char * component_type, const char * path)
        : ZstEntityBase(path)
    {
        set_entity_type(ZstEntityType::COMPONENT);
        set_component_type(component_type);
    }
    
    ZstComponent::ZstComponent(const Component* buffer) : 
		ZstEntityBase()
    {
		set_entity_type(ZstEntityType::COMPONENT);
        ZstEntityBase::deserialize_partial(buffer->entity());
		ZstComponent::deserialize_partial(buffer->component());
    }

    ZstComponent::ZstComponent(const ZstComponent & other) : ZstEntityBase(other)
    {
        for (auto c : other.m_children) {
			auto entity = get_child_by_URI(c);

            if (entity->entity_type() == ZstEntityType::COMPONENT) {
                add_child(new ZstComponent(*dynamic_cast<ZstComponent*>(entity)));
            }
            else if(entity->entity_type() == ZstEntityType::PLUG) {
                ZstPlug * plug = dynamic_cast<ZstPlug*>(entity);
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
        ZstEntityBundle bundle;
        /*get_child_entities(bundle, false);
        for (auto child : bundle) {
			child->deactivate();
        }*/
        m_children.clear();
    }

    ZstEntityBundle & ZstComponent::get_plugs(ZstEntityBundle & bundle) const
    {
        for(auto entity_path : m_children){
			auto entity = get_child_by_URI(entity_path);
			if (!entity)
				continue;
            if(entity->entity_type() ==ZstEntityType::PLUG)
                bundle.add(entity);
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
		m_children.emplace(entity->URI());

		if (is_registered() && !entity->is_registered()) {
			entity_events()->invoke([entity](std::shared_ptr<ZstEntityAdaptor> adaptor) {
				adaptor->on_request_entity_registration(entity);
			});
		}
        
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

	uoffset_t ZstComponent::serialize(FlatBufferBuilder & buffer_builder) const
    {
        Offset<ComponentData> component_offset;
        serialize_partial(component_offset, buffer_builder);
        
		Offset<EntityData> entity_offset;
		ZstEntityBase::serialize_partial(entity_offset, buffer_builder);
		
        return CreateComponent(buffer_builder, entity_offset, component_offset).o;
    }
    
    void ZstComponent::serialize_partial(flatbuffers::Offset<ComponentData>& serialized_offset, FlatBufferBuilder& buffer_builder) const
    {
        auto component_type_offset = buffer_builder.CreateString(m_component_type.c_str(), m_component_type.size());
        serialized_offset = CreateComponentData(buffer_builder, component_type_offset);
    }

    void ZstComponent::deserialize(const Component* buffer)
    {
        ZstComponent::deserialize_partial(buffer->component());
        ZstEntityBase::deserialize_partial(buffer->entity());
    }
    
    void ZstComponent::deserialize_partial(const ComponentData* buffer)
    {
        if (!buffer) return;
        m_component_type = (buffer->component_type()) ? buffer->component_type()->str() : "";
    }

    const char * ZstComponent::component_type() const
    {
        return m_component_type.c_str();
    }

    void ZstComponent::on_registered()
    {
    }

    void ZstComponent::on_activation()
    {
    }

    void ZstComponent::on_deactivation()
    {
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
		// Add the root object first so the bundle preserves ancestor-first order
		if (include_parent)
			bundle.add(this);

        for (auto child_path : m_children) {
			auto entity = get_child_by_URI(child_path);
			if(entity)
				entity->get_child_entities(bundle);
        }
        ZstEntityBase::get_child_entities(bundle, false);
    }



    //--------------

    ZstEntityBase * ZstComponent::walk_child_by_URI(const ZstURI & path) const
    {
        ZstEntityBase * result = NULL;
        const ZstEntityBase * previous = NULL;
        
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
            const ZstComponent * prev_container = dynamic_cast<const ZstComponent*>(previous);
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

    ZstEntityBase * ZstComponent::get_child_by_URI(const ZstURI & path) const
    {
        ZstEntityBase * result = NULL;

		if (!is_registered()) {
			ZstLog::entity(LogLevel::warn, "Entity {} not registered. Can't look up child entity.", URI().path());
		}

        m_hierarchy_events->invoke([&result, &path](std::shared_ptr<ZstHierarchyAdaptor> adaptor) {
			result = adaptor->find_entity(path);
		});
        
        return result;
    }

    const size_t ZstComponent::num_children() const
    {
        return m_children.size();
    }
}
