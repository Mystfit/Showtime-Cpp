#pragma once

#include <unordered_map>
#include <memory>

#include "ZstExports.h"
#include "ZstConstants.h"
#include "ZstURI.h"
#include "entities/ZstEntityFactory.h"
#include "entities/ZstComponent.h"

namespace showtime
{
class ZST_CLASS_EXPORTED ZstPerformer :
	public virtual ZstSerialisable<Performer, PerformerData>,
    public ZstComponent
{
public:
    ZST_EXPORT ZstPerformer();
    ZST_EXPORT ZstPerformer(const char * name);
    ZST_EXPORT ZstPerformer(const Performer* buffer);
    ZST_EXPORT ZstPerformer(const ZstPerformer & other);
    ZST_EXPORT ~ZstPerformer();
    
    //Heartbeat

    ZST_EXPORT void set_heartbeat_active();
    ZST_EXPORT void set_heartbeat_inactive();
    ZST_EXPORT void clear_active_hearbeat();
    ZST_EXPORT bool get_active_heartbeat();
    ZST_EXPORT int get_missed_heartbeats();

    //Hierarchy

    ZST_EXPORT void add_child(ZstEntityBase * entity, bool auto_activate = true) override;
    ZST_EXPORT void remove_child(ZstEntityBase * entity) override;
    ZST_EXPORT void add_factory(ZstEntityFactory * factory);
    ZST_EXPORT void remove_factory(ZstEntityFactory * factory);
    ZST_EXPORT ZstEntityFactoryBundle & get_factories(ZstEntityFactoryBundle & bundle);
    
	// Serialisation

    ZST_EXPORT virtual void serialize_partial(flatbuffers::Offset<PerformerData>& destination_offset, flatbuffers::FlatBufferBuilder & buffer_builder) const override;
	ZST_EXPORT virtual flatbuffers::uoffset_t serialize(flatbuffers::FlatBufferBuilder& buffer_builder) const override;
    ZST_EXPORT virtual void deserialize_partial(const PerformerData* buffer) override;
	ZST_EXPORT virtual void deserialize(const Performer* buffer) override;

private:

    //Heartbeat status
    bool m_heartbeat_active;
    int m_missed_heartbeats;
};

typedef std::unordered_map<ZstURI, ZstPerformer*, ZstURIHash> ZstPerformerMap;
}
