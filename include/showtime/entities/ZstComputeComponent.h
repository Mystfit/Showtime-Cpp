#pragma once

#include <vector>
#include <stack>

#include <flatbuffers/flatbuffers.h>

#include <showtime/schemas/messaging/graph_types_generated.h>
#include <showtime/ZstExports.h>
#include <showtime/entities/ZstComponent.h>
#include <showtime/entities/ZstPlug.h>
#include <showtime/ZstCable.h>

namespace showtime
{
class ZST_CLASS_EXPORTED ZstComputeComponent :
    public ZstComponent
{
public:
    ZST_EXPORT ZstComputeComponent();
    ZST_EXPORT ZstComputeComponent(const char* path);
    ZST_EXPORT ZstComputeComponent(const char* component_type, const char* path);
    ZST_EXPORT ZstComputeComponent(const Component* buffer);
    ZST_EXPORT ZstComputeComponent(const ZstComponent& other);

    ZST_EXPORT virtual void on_registered() override;
    ZST_EXPORT virtual void on_child_cable_connected(ZstCable* cable) override;
    ZST_EXPORT virtual void on_child_cable_disconnected(const ZstCableAddress& cable_address) override;

    // Start an execution chain to trigger compute() on each upstream component to resolve this component
    ZST_EXPORT void execute_upstream();

    // Cache the execution order to avoid recalculation
    ZST_EXPORT void cache_execution_order();
    ZST_EXPORT void clear_execution_order_cache();

    // Helpers to retrieve execution plugs
    ZST_EXPORT ZstInputPlug* get_upstream_compute_plug();
    ZST_EXPORT ZstOutputPlug* get_downstream_compute_plug();

    //Overridable compute function that will process input plug events
    ZST_EXPORT virtual void compute();
    ZST_EXPORT virtual void compute(ZstInputPlug* plug);

    // Flag this compute component as needing to execute due to changed inputs
    ZST_EXPORT void set_execution_order_dirty();

private:
    ZstURIBundle m_cached_execution_order;
    bool m_execution_order_dirty;

    // Compute chain plugs
    std::shared_ptr<ZstOutputPlug> m_compute_outgoing_plug;
    std::shared_ptr<ZstInputPlug> m_compute_incoming_plug;
    void init_compute_plugs();
};
}
