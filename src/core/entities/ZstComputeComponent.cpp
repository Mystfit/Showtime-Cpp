#include <memory>
#include <showtime/entities/ZstComputeComponent.h>
#include <showtime/adaptors/ZstEntityAdaptor.hpp>
#include "../ZstEventDispatcher.hpp"

using namespace flatbuffers;

namespace showtime
{
    ZstComputeComponent::ZstComputeComponent() :
        ZstComponent(),
        m_execution_order_dirty(true),
        m_compute_outgoing_plug(nullptr),
        m_compute_incoming_plug(nullptr)
    {
        set_entity_type(ZstEntityType::COMPONENT);
        set_component_type("");
        init_compute_plugs();
    }

    ZstComputeComponent::ZstComputeComponent(const char* path) :
        ZstComponent(path),
        m_execution_order_dirty(true),
        m_compute_outgoing_plug(nullptr),
        m_compute_incoming_plug(nullptr)
    {
        set_entity_type(ZstEntityType::COMPONENT);
        set_component_type("");
        init_compute_plugs();
    }

    ZstComputeComponent::ZstComputeComponent(const char* component_type, const char* path) :
        ZstComponent(component_type, path),
        m_execution_order_dirty(true),
        m_compute_outgoing_plug(nullptr),
        m_compute_incoming_plug(nullptr)
    {
        set_entity_type(ZstEntityType::COMPONENT);
        set_component_type(component_type);
        init_compute_plugs();
    }

    ZstComputeComponent::ZstComputeComponent(const Component* buffer) :
        ZstComponent(buffer),
        m_execution_order_dirty(true),
        m_compute_outgoing_plug(nullptr),
        m_compute_incoming_plug(nullptr)
    {
        set_entity_type(ZstEntityType::COMPONENT);
        ZstEntityBase::deserialize_partial(buffer->entity());
        ZstComponent::deserialize_partial(buffer->component());
        init_compute_plugs();
    }

    ZstComputeComponent::ZstComputeComponent(const ZstComponent& other) : ZstComponent(other)
    {
        m_execution_order_dirty = true;
        init_compute_plugs();
    }

    void ZstComputeComponent::on_registered()
    {
        ZstComputeComponent::on_registered();
        add_child(m_compute_incoming_plug.get());
        add_child(m_compute_outgoing_plug.get());
    }

    void ZstComputeComponent::on_child_cable_connected(ZstCable* cable)
    {
        clear_execution_order_cache();
    }

    void ZstComputeComponent::on_child_cable_disconnected(const ZstCableAddress& cable_address)
    {
        clear_execution_order_cache();
    }

    void ZstComputeComponent::execute_upstream()
    {
        // Create a random ID for this execution chain
        m_compute_outgoing_plug->append_int(rand());
        m_compute_outgoing_plug->fire();

        if (m_execution_order_dirty)
            cache_execution_order();

        ZstEntityBundle bundle;
        {
            //std::lock_guard<std::mutex> lock(m_entity_mtx);
            for (auto entity_path : m_cached_execution_order) {
                if (auto entity = find_entity(entity_path)) {
                    bundle.add(entity);
                }
            }
        }

        for (auto entity : bundle) {
            if (entity->entity_type() == ZstEntityType::COMPONENT) {
                auto component = static_cast<ZstComputeComponent*>(entity);
                component->compute();
            }
        }
    }

    void ZstComputeComponent::init_compute_plugs()
    {
        m_compute_incoming_plug = std::make_shared<ZstInputPlug>("execute", ZstValueType::IntList, -1, true);
        m_compute_outgoing_plug = std::make_shared<ZstOutputPlug>("then", ZstValueType::IntList, 1);
    }

    void ZstComputeComponent::cache_execution_order()
    {
        m_execution_order_dirty = false;

        ZstEntityBundle execution_order;
        dependencies(&execution_order, true);

        //std::lock_guard<std::mutex> lock(m_entity_mtx);
        m_cached_execution_order.clear();
        for (auto entity : execution_order) {
            // Only cache computable entities
            if (auto compute_comp = dynamic_cast<ZstComputeComponent*>(entity)) {
                m_cached_execution_order.add(entity->URI());
            }
        }
    }

    void ZstComputeComponent::clear_execution_order_cache()
    {
        ZstEntityBundle bundle;
        {
            //std::lock_guard<std::mutex> lock(m_entity_mtx);
            for (auto entity_path : m_cached_execution_order) {
                if (auto entity = find_entity(entity_path)) {
                    bundle.add(entity);
                }
            }
        }

        for (auto entity : bundle) {
            if (entity->entity_type() == ZstEntityType::COMPONENT) {
                if (auto compute_comp = dynamic_cast<ZstComputeComponent*>(entity)) {
                    compute_comp->set_execution_order_dirty();
                }
            }
        }

        m_cached_execution_order.clear();
    }

    ZstInputPlug* ZstComputeComponent::get_upstream_compute_plug()
    {
        if (m_compute_incoming_plug)
            return m_compute_incoming_plug.get();
        return nullptr;
    }

    ZstOutputPlug* ZstComputeComponent::get_downstream_compute_plug()
    {
        if (m_compute_outgoing_plug)
            return m_compute_outgoing_plug.get();
        return nullptr;
    }

    void ZstComputeComponent::compute()
    {
        compute(get_upstream_compute_plug());
    }

    void ZstComputeComponent::compute(ZstInputPlug* plug)
    {
        entity_event_dispatcher()->invoke([plug](ZstEntityAdaptor* adaptor) {
            adaptor->on_compute(plug);
        });

        if (plug == m_compute_incoming_plug.get()) {
            int compute_id = (m_compute_incoming_plug->size() > 0) ? m_compute_incoming_plug->int_at(0) : -1;
            m_compute_outgoing_plug->append_int(compute_id);
            m_compute_outgoing_plug->fire();
        }
    }

    void ZstComputeComponent::set_execution_order_dirty()
    {
        m_execution_order_dirty = true;
    }
}
