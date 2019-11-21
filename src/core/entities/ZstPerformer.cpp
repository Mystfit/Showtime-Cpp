#include <exception>
#include "entities/ZstPerformer.h"
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
    set_entity_type(EntityTypes_Performer);
}

ZstPerformer::ZstPerformer(const char * name) :
    ZstComponent("", name),
    m_heartbeat_active(false),
    m_missed_heartbeats(0)
{
    set_entity_type(EntityTypes_Performer);
}
    
ZstPerformer::ZstPerformer(const Performer* buffer) : 
	ZstComponent()
{
	set_entity_type(EntityTypes_Performer);
	ZstPerformer::deserialize_partial(buffer->performer());
	ZstComponent::deserialize_partial(buffer->component());
	ZstEntityBase::deserialize_partial(buffer->entity());
}

ZstPerformer::ZstPerformer(const ZstPerformer & other) : ZstComponent(other)
{
    m_heartbeat_active = other.m_heartbeat_active;
    m_missed_heartbeats = other.m_missed_heartbeats;
    
    for (auto f : other.m_factories) {
        add_factory(new ZstEntityFactory(*(f.second)));
    }
}

ZstPerformer::~ZstPerformer()
{
    if (!is_proxy()){
        for (auto f : m_factories) {
            // TODO: Deleting factories will crash the host if it GC's factories after the performer has been freed
            ZstLog::entity(LogLevel::debug, "FIXME: Performer {} leaking factory {} to avoid host app crashing when GCing", URI().path(), f.second->URI().path());
            //delete f.second;
        }
        m_factories.clear();
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
    if (entity->entity_type() == EntityTypes_Factory) {
        add_factory(static_cast<ZstEntityFactory*>(entity));
    }
    else {
        ZstComponent::add_child(entity, auto_activate);
    }
}

void ZstPerformer::remove_child(ZstEntityBase * entity)
{
    if (entity->entity_type() == EntityTypes_Factory) {
        remove_factory(static_cast<ZstEntityFactory*>(entity));
    }
    else {
        ZstComponent::remove_child(entity);
    }
}

void ZstPerformer::add_factory(ZstEntityFactory * factory)
{
    if (is_destroyed()) return;

    ZstEntityBase::add_child(factory);
    m_factories[factory->URI()] = factory;
}

void ZstPerformer::remove_factory(ZstEntityFactory * factory)
{
    auto f = m_factories.find(factory->URI());
    if (f != m_factories.end()) {
        m_factories.erase(f);
    }

    ZstEntityBase::remove_child(factory);
}

ZstEntityBundle & ZstPerformer::get_factories(ZstEntityBundle & bundle)
{
    for (auto f : m_factories) {
        bundle.add(f.second);
    }
    return bundle;
}

ZstEntityFactoryBundle & ZstPerformer::get_factories(ZstEntityFactoryBundle & bundle)
{
    for (auto f : m_factories) {
        bundle.add(f.second);
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
    ZstPerformer::deserialize_partial(buffer->performer());
    ZstComponent::deserialize_partial(buffer->component());
    ZstEntityBase::deserialize_partial(buffer->entity());
}
    
void ZstPerformer::deserialize_partial(const PerformerData* buffer)
{
}

}
