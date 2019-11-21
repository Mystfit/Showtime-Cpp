#pragma once

#include <vector>
#include <flatbuffers/flatbuffers.h>
#include <schemas/graph_types_generated.h>

#include "ZstExports.h"
#include "entities/ZstEntityBase.h"
#include "entities/ZstPlug.h"
#include "ZstCable.h"

namespace showtime
{
class ZST_CLASS_EXPORTED ZstComponent :
	public virtual ZstSerialisable<Component, ComponentData>,
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
    ZST_EXPORT ZstEntityBundle & get_plugs(ZstEntityBundle & bundle) const;

    //Overridable compute function that will process input plug events
    ZST_EXPORT virtual void compute(ZstInputPlug * plug) {};
    
    //Create and attach a new input plug to this component
    ZST_EXPORT ZstInputPlug * create_input_plug(const char* name, ValueList val_type);
    ZST_EXPORT ZstInputPlug * create_input_plug(const char* name, ValueList val_type, int max_cable_connections);

    //Create and attach a new output plug to this component
    ZST_EXPORT ZstOutputPlug * create_output_plug(const char* name, ValueList val_type, bool reliable = true);

    //Transfer plug ownership to this component
    ZST_EXPORT virtual void add_child(ZstEntityBase * entity, bool auto_activate = true) override;

    //Remove a plug from this component
    ZST_EXPORT virtual void remove_child(ZstEntityBase * entity) override;

    //Hierarchy
    ZST_EXPORT virtual void get_child_cables(ZstCableBundle & bundle) override;
    ZST_EXPORT virtual void get_child_entities(ZstEntityBundle & bundle, bool include_parent = true) override;
    
    //Specific component type
    ZST_EXPORT const char * component_type() const;


	// Serialisation
	// -------------
    using ZstEntityBase::serialize;
    using ZstEntityBase::serialize_partial;
    using ZstEntityBase::deserialize;
    using ZstEntityBase::deserialize_partial;
    ZST_EXPORT virtual void serialize_partial(flatbuffers::Offset<ComponentData>& destination_offset, flatbuffers::FlatBufferBuilder & buffer_builder) const override;
	ZST_EXPORT virtual flatbuffers::uoffset_t serialize(flatbuffers::FlatBufferBuilder& buffer_builder) const override;
    ZST_EXPORT virtual void deserialize_partial(const ComponentData* buffer) override;
	ZST_EXPORT virtual void deserialize(const Component* buffer) override;

    
    // Children
    // --------

    //Find a child in this entity by a URI
    ZST_EXPORT ZstEntityBase * walk_child_by_URI(const ZstURI & path);
    
    //Get a child entity at a specific URI
    ZST_EXPORT ZstEntityBase * get_child_by_URI(const ZstURI & path);
    
    //Get a child by index
    ZST_EXPORT ZstEntityBase * get_child_at(size_t index) const;
    
    //Number of children owned by this entity
    ZST_EXPORT const size_t num_children() const;


protected:
    ZST_EXPORT void set_component_type(const char * component_type);
    ZST_EXPORT void set_component_type(const char * component_type, size_t len);

    //Set parent of this component
    ZST_EXPORT virtual void set_parent(ZstEntityBase * parent) override;
    
private:
    ZstEntityMap m_children;
    std::string m_component_type;
};
}
