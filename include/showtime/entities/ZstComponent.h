#pragma once

#include <vector>
#include <stack>

#include <flatbuffers/flatbuffers.h>

#include <showtime/schemas/messaging/graph_types_generated.h>
#include <showtime/ZstExports.h>
#include <showtime/entities/ZstEntityBase.h>
#include <showtime/entities/ZstPlug.h>
#include <showtime/ZstCable.h>

namespace showtime
{
class ZST_CLASS_EXPORTED ZstComponent :
#ifndef SWIG
	public virtual ZstSerialisable<Component, ComponentData>,
#endif
    public ZstEntityBase
{
public:
    ZST_EXPORT ZstComponent();
    ZST_EXPORT ZstComponent(const char * path);
    ZST_EXPORT ZstComponent(const char * component_type, const char * path);
    ZST_EXPORT ZstComponent(const Component* buffer);
    ZST_EXPORT ZstComponent(const ZstComponent & other);

    ZST_EXPORT virtual ~ZstComponent();
    
    //External factory function
    ZST_EXPORT virtual void create(const char * name, ZstEntityBase* parent) {};
    
    //Find a plug in this component by its URI
    ZST_EXPORT void get_plugs(ZstEntityBundle* bundle);

    // Start an execution chain to trigger compute() on each downstream component
    ZST_EXPORT void execute();

    // Cache the execution order to avoid recalculation
    ZST_EXPORT void cache_execution_order();
    ZST_EXPORT void clear_execution_order_cache();

    ZST_EXPORT ZstInputPlug* get_upstream_compute_plug();
    ZST_EXPORT ZstOutputPlug* get_downstream_compute_plug();

    //Overridable compute function that will process input plug events
    ZST_EXPORT virtual void compute(ZstInputPlug * plug);

    //Transfer plug ownership to this component
    ZST_EXPORT virtual void add_child(ZstEntityBase * entity, bool auto_activate = true) override;

    //Remove a plug from this component
    ZST_EXPORT virtual void remove_child(ZstEntityBase * entity) override;

    // Change and update the name of this entity
    ZST_EXPORT virtual void set_name(const char * name) override;

    //Hierarchy
    ZST_EXPORT virtual void get_child_cables(ZstCableBundle* bundle) override;
    ZST_EXPORT virtual void get_child_entities(ZstEntityBundle* bundle, bool include_parent = false, bool recursive = false, ZstEntityType filter = ZstEntityType::NONE) override;
    
    // Cable queries
    ZST_EXPORT void dependants(ZstEntityBundle* out_entities, bool recursive = false, bool local_only = true);
    ZST_EXPORT void dependencies(ZstEntityBundle* out_entities, bool recursive = false, bool local_only = true);
    ZST_EXPORT void directed_graph(ZstEntityBundle* out_entities, ZstPlugDirection direction, bool recursive, bool local_only);
    ZST_EXPORT void get_adjacent_components(ZstEntityBundle* entities, ZstPlugDirection direction);

    //Specific component type
    ZST_EXPORT const char * component_type() const;


    // Overidden Events for SWIG
    ZST_EXPORT virtual void on_registered() override;
    ZST_EXPORT virtual void on_activation() override;
    ZST_EXPORT virtual void on_deactivation() override;


	// Serialisation
	// -------------
    using ZstEntityBase::serialize;
    using ZstEntityBase::deserialize;
    using ZstEntityBase::serialize_partial;
    using ZstEntityBase::deserialize_partial;
    ZST_EXPORT virtual void serialize_partial(flatbuffers::Offset<ComponentData>& destination_offset, flatbuffers::FlatBufferBuilder & buffer_builder) const override;
	ZST_EXPORT virtual flatbuffers::uoffset_t serialize(flatbuffers::FlatBufferBuilder& buffer_builder) const override;
    ZST_EXPORT virtual void deserialize_partial(const ComponentData* buffer) override;
	ZST_EXPORT virtual void deserialize(const Component* buffer) override;

    
    // Children
    // --------

    //Find a child in this entity by a URI
    ZST_EXPORT ZstEntityBase * walk_child_by_URI(const ZstURI & path) const;
    
    //Get a child entity at a specific URI
    ZST_EXPORT ZstEntityBase * get_child_by_URI(const ZstURI & path) const;
        
    //Number of children owned by this entity
    ZST_EXPORT const size_t num_children() const;


protected:
    ZST_EXPORT void set_component_type(const char * component_type);
    ZST_EXPORT void set_component_type(const char * component_type, size_t len);

    //Set parent of this component
    ZST_EXPORT virtual void set_parent(ZstEntityBase * parent) override;
    
    ZST_EXPORT void set_execution_order_dirty();
    
private:
	std::set<ZstURI> m_children;
    std::string m_component_type;
    bool m_triggers_compute;
    ZstURIBundle m_cached_execution_order;
    bool m_execution_order_dirty;

    // Compute chain plugs
    void init_compute_plugs();
    std::shared_ptr<ZstOutputPlug> m_compute_outgoing_plug;
    std::shared_ptr<ZstInputPlug> m_compute_incoming_plug;

    void computeTopologicalSort(ZstComponent* vertex, std::set<ZstComponent*>& visited, std::stack<ZstComponent*>& stack, ZstPlugDirection direction);
};
}
