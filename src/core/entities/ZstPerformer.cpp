#include <exception>
#include <showtime/entities/ZstPerformer.h>
#include <showtime/ZstLogging.h>
#include "../liasons/ZstEntityLiason.hpp"
#include "../ZstEventDispatcher.hpp"

using namespace std;
using namespace flatbuffers;

namespace showtime
{
ZstPerformer::ZstPerformer() :
    ZstComponent(),
    m_heartbeat_active(false),
    m_missed_heartbeats(0)
{
    set_entity_type(ZstEntityType::PERFORMER);
}

ZstPerformer::ZstPerformer(const char * name) :
    ZstComponent("", name),
    m_heartbeat_active(false),
    m_missed_heartbeats(0)
{
    set_entity_type(ZstEntityType::PERFORMER);
}
    
ZstPerformer::ZstPerformer(const Performer* buffer) : 
	ZstComponent(),
	m_missed_heartbeats(0)

{
	set_entity_type(ZstEntityType::PERFORMER);
	ZstPerformer::deserialize_partial((buffer->performer()) ? buffer->performer() : NULL);
	ZstComponent::deserialize_partial((buffer->component()) ? buffer->component() : NULL);
	ZstEntityBase::deserialize_partial(buffer->entity());
}

ZstPerformer::ZstPerformer(const ZstPerformer & other) : ZstComponent(other)
{
    m_heartbeat_active = other.m_heartbeat_active;
    m_missed_heartbeats = other.m_missed_heartbeats;
}

ZstPerformer::~ZstPerformer()
{
    if (!is_proxy()){
        /*auto bundle = ZstEntityBundle();
        get_factories(bundle);
        for (auto f : bundle) {*/
            // TODO: Deleting factories will crash the host if it GC's factories after the performer has been freed
            //delete f.second;
            //ZstEntityLiason().entity_set_parent(f, NULL);
            //f->deactivate();
        //}
        //m_factories.clear();
    }
}

void ZstPerformer::set_heartbeat_active()
{
    m_heartbeat_active = true;
    m_missed_heartbeats = 0;
}

void ZstPerformer::clear_active_hearbeat() {
    m_heartbeat_active = false;
}

bool ZstPerformer::get_active_heartbeat()
{
    return m_heartbeat_active;
}

void ZstPerformer::set_heartbeat_inactive()
{
    m_missed_heartbeats++;
}

int ZstPerformer::get_missed_heartbeats()
{
    return m_missed_heartbeats;
}

void ZstPerformer::add_child(ZstEntityBase * entity, bool auto_activate)
{
    if (entity->entity_type() == ZstEntityType::FACTORY) {
        add_factory(static_cast<ZstEntityFactory*>(entity));
    }
    else {
        ZstComponent::add_child(entity, auto_activate);
    }
}

void ZstPerformer::remove_child(ZstEntityBase * entity)
{
    if (entity->entity_type() == ZstEntityType::FACTORY) {
        remove_factory(static_cast<ZstEntityFactory*>(entity));
    }
    else {
        ZstComponent::remove_child(entity);
    }
}

void ZstPerformer::add_factory(ZstEntityFactory * factory)
{
    if (is_destroyed()) return;
    ZstComponent::add_child(factory);
}

void ZstPerformer::remove_factory(ZstEntityFactory * factory)
{
    if (is_destroyed()) return;
    ZstComponent::remove_child(factory);
}

ZstEntityFactoryBundle & ZstPerformer::get_factories(ZstEntityFactoryBundle & bundle)
{
    auto entity_bundle = ZstEntityBundle();
    get_child_entities(entity_bundle, false, false, ZstEntityType::FACTORY);
    for (auto entity : entity_bundle) {
        bundle.add(dynamic_cast<ZstEntityFactory*>(entity));
    }
    return bundle;
}
    
void ZstPerformer::serialize_partial(flatbuffers::Offset<PerformerData>& serialized_offset, FlatBufferBuilder& buffer_builder) const
{
    serialized_offset = CreatePerformerData(buffer_builder);
}

uoffset_t ZstPerformer::serialize(FlatBufferBuilder& buffer_builder) const
{
	Offset<ComponentData> component_offset;
	ZstComponent::serialize_partial(component_offset, buffer_builder);
    
    Offset<EntityData> entity_offset;
    ZstEntityBase::serialize_partial(entity_offset, buffer_builder);
    
    Offset<PerformerData> performer_offset;
    serialize_partial(performer_offset, buffer_builder);
    
	return CreatePerformer(buffer_builder, entity_offset, component_offset, performer_offset).o;
}

void ZstPerformer::deserialize(const Performer* buffer)
{
    if (!buffer) return;

    ZstPerformer::deserialize_partial(buffer->performer());
    ZstComponent::deserialize_partial(buffer->component());
    ZstEntityBase::deserialize_partial(buffer->entity());
}
    
void ZstPerformer::deserialize_partial(const PerformerData* buffer)
{
}

}
