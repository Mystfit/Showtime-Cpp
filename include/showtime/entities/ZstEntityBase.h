#pragma once

#include <unordered_map>
#include <mutex>
#include <memory>

#include "../ZstExports.h"
#include "../ZstURI.h"
#include "../ZstSerialisable.h"
#include "../ZstSynchronisable.h"
#include "../ZstBundle.hpp"
#include "../ZstCable.h"
#include "../ZstCableAddress.h"
#include "../ZstBundle.hpp"
#include "../adaptors/ZstEntityAdaptor.hpp"
#include "../adaptors/ZstSessionAdaptor.hpp"
#include "../adaptors/ZstHierarchyAdaptor.hpp"


namespace showtime
{

//Forwards
template<typename T>
class ZstEventDispatcher;
class ZstEntityBase;
class ZstEntityFactory;

//Typedefs
typedef std::unordered_map<ZstURI, ZstEntityBase*, ZstURIHash> ZstEntityMap;

//Common entity bundle types
typedef ZstBundle<ZstURI> ZstURIBundle;
typedef ZstBundle<ZstEntityBase*> ZstEntityBundle;
typedef ZstBundle<ZstEntityFactory*> ZstEntityFactoryBundle;
typedef ZstBundleIterator<ZstEntityBase*> ZstEntityBundleIterator;


enum class ZstEntityType {
    NONE = 0,
    COMPONENT,
    PERFORMER,
    PLUG,
    FACTORY
};


class ZST_CLASS_EXPORTED ZstEntityBase :
#ifndef SWIG
    public virtual ZstSerialisable<Entity, EntityData>,
#endif
    public ZstSynchronisable
{
    friend class ZstEntityLiason;

public:
    //Base entity
    ZST_EXPORT ZstEntityBase();
    ZST_EXPORT ZstEntityBase(const char * entity_name);
    ZST_EXPORT ZstEntityBase(const EntityData* buffer);
    ZST_EXPORT ZstEntityBase(const ZstEntityBase & other);
    ZST_EXPORT virtual ~ZstEntityBase();
    
    //The parent of this entity
    ZST_EXPORT ZstEntityBase * parent() const;
	ZST_EXPORT const ZstURI& parent_address() const;

    ZST_EXPORT virtual void add_child(ZstEntityBase * child, bool auto_activate = true);
    ZST_EXPORT virtual void remove_child(ZstEntityBase * child);
    
    //Entity type
    ZST_EXPORT const ZstEntityType entity_type() const;
    ZST_EXPORT EntityTypes serialized_entity_type();
    
    //URI for this entity
    ZST_EXPORT const ZstURI & URI() const;
    ZST_EXPORT virtual void set_name(const char* name);

    //Iterate
    ZST_EXPORT virtual void get_child_cables(ZstCableBundle* bundle);
    ZST_EXPORT virtual void get_child_entities(ZstEntityBundle* bundle, bool include_parent = false, bool recursive = false, ZstEntityType filter = ZstEntityType::NONE);

    //Serialisation
    ZST_EXPORT virtual void serialize_partial(flatbuffers::Offset<EntityData>& destination_offset, flatbuffers::FlatBufferBuilder & buffer_builder) const override;
    ZST_EXPORT virtual flatbuffers::uoffset_t serialize(flatbuffers::FlatBufferBuilder & buffer_builder) const override;
    ZST_EXPORT virtual void deserialize_partial(const EntityData* buffer) override;
	ZST_EXPORT virtual void deserialize(const Entity* buffer) override;

    //Adaptors
    ZST_EXPORT virtual void add_adaptor(std::shared_ptr<ZstEntityAdaptor> adaptor);
    ZST_EXPORT virtual void add_adaptor(std::shared_ptr<ZstSessionAdaptor> adaptor);
	ZST_EXPORT virtual void add_adaptor(std::shared_ptr<ZstHierarchyAdaptor> adaptor);
    ZST_EXPORT virtual void remove_adaptor(std::shared_ptr<ZstEntityAdaptor> adaptor);
    ZST_EXPORT virtual void remove_adaptor(std::shared_ptr<ZstSessionAdaptor> adaptor);
	ZST_EXPORT virtual void remove_adaptor(std::shared_ptr<ZstHierarchyAdaptor> adaptor);
    
    //Ownership
    ZST_EXPORT const ZstURI & get_owner() const;
    ZST_EXPORT void aquire_ownership();
    ZST_EXPORT void release_ownership();

	//Registration
    ZST_EXPORT virtual void on_registered();
	ZST_EXPORT bool is_registered() const;

	//Activation
	ZST_EXPORT virtual void activate();
	ZST_EXPORT virtual void activate_async();
	ZST_EXPORT virtual void deactivate();
	ZST_EXPORT virtual void deactivate_async();

#ifndef SWIG
    //Include base class adaptors
    //Swig mistakenly adds these twice when dealing treating ZstSynchronisable as an interface (C#, Java)
    using ZstSynchronisable::add_adaptor;
    using ZstSynchronisable::remove_adaptor;
#endif
    ZST_EXPORT ZstEntityAdaptor* entity_events();
    
protected:
    //Set entity status
    ZST_EXPORT void set_entity_type(ZstEntityType entity_type);
    ZST_EXPORT virtual void set_parent(ZstEntityBase* entity);
    ZST_EXPORT virtual void set_owner(const ZstURI & fire_owner);
    ZST_EXPORT virtual void update_URI(const ZstURI& original_path);
    ZST_EXPORT virtual void dispatch_destroyed() override;
    
    //Event dispatchers
    ZST_EXPORT std::shared_ptr<ZstEventDispatcher<ZstEntityAdaptor> >& entity_event_dispatcher();
    ZST_EXPORT std::shared_ptr<ZstEventDispatcher<ZstSessionAdaptor> > & session_events();
	ZST_EXPORT std::shared_ptr<ZstEventDispatcher<ZstHierarchyAdaptor> >& hierarchy_events();

    //Entity mutex
    mutable std::mutex m_entity_mtx;
    std::shared_ptr<ZstEventDispatcher<ZstSessionAdaptor> > m_session_events;
	std::shared_ptr<ZstEventDispatcher<ZstHierarchyAdaptor> > m_hierarchy_events;
    std::shared_ptr<ZstEventDispatcher<ZstEntityAdaptor> > m_entity_events;

private:
	void set_registered(bool registered);
    ZstURI m_parent;
    ZstEntityType m_entity_type;
    ZstURI m_uri;
    ZstURI m_current_owner;
    std::mutex m_entity_lock;
	bool m_registered;
};
}
