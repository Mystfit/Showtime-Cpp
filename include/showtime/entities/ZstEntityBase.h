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


class ZST_CLASS_EXPORTED ZstEntityBase : public ZstSynchronisable, public ZstSerialisable {
    friend class ZstEntityLiason;

public:
	//Base entity
	ZST_EXPORT ZstEntityBase(const char * entity_name);
	ZST_EXPORT ZstEntityBase(const ZstEntityBase & other);
	ZST_EXPORT virtual ~ZstEntityBase();
    	
	//The parent of this entity
	ZST_EXPORT ZstEntityBase * parent() const;

	ZST_EXPORT virtual void add_child(ZstEntityBase * child, bool auto_activate = true);
	ZST_EXPORT virtual void remove_child(ZstEntityBase * child);
    
    //Entity type
	ZST_EXPORT const char * entity_type() const;
    
    //URI for this entity
	ZST_EXPORT const ZstURI & URI() const;

	//Iterate
	ZST_EXPORT virtual void get_child_cables(ZstCableBundle & bundle);
	ZST_EXPORT virtual void get_child_entities(ZstEntityBundle & bundle, bool include_parent = true);
	    
	//Serialisation
	ZST_EXPORT void write_json(json & buffer) const override;
	ZST_EXPORT void read_json(const json & buffer) override;

	//Adaptors
    ZST_EXPORT virtual void add_adaptor(std::shared_ptr<ZstEntityAdaptor>& adaptor);
    ZST_EXPORT virtual void add_adaptor(std::shared_ptr<ZstSessionAdaptor>& adaptor);
    ZST_EXPORT virtual void remove_adaptor(std::shared_ptr<ZstEntityAdaptor>& adaptor);
    ZST_EXPORT virtual void remove_adaptor(std::shared_ptr<ZstSessionAdaptor>& adaptor);
    
    //Ownership
    ZST_EXPORT const ZstURI & get_owner() const;
    ZST_EXPORT void aquire_ownership();
    ZST_EXPORT void release_ownership();
#ifndef SWIG
    //Include base class adaptors
    //Swig mistakenly adds these twice when dealing treating ZstSynchronisable as an interface (C#, Java)
    using ZstSynchronisable::add_adaptor;
    using ZstSynchronisable::remove_adaptor;
#endif

	ZST_EXPORT std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstEntityAdaptor> > > & entity_events();
	
protected:
	//Set entity status
	ZST_EXPORT void set_entity_type(const char * entity_type);
	ZST_EXPORT virtual void set_parent(ZstEntityBase* entity);
    ZST_EXPORT virtual void set_owner(const ZstURI & fire_owner);
	ZST_EXPORT virtual void update_URI();
	ZST_EXPORT virtual void dispatch_destroyed() override;
    
    //Event dispatchers
    ZST_EXPORT std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstSessionAdaptor> > > & session_events();
    
    //Entity mutex
    mutable std::mutex m_entity_mtx;
	std::shared_ptr<ZstEventDispatcher< std::shared_ptr< ZstSessionAdaptor> > > m_session_events;
	std::shared_ptr<ZstEventDispatcher< std::shared_ptr< ZstEntityAdaptor> > > m_entity_events;

private:
	ZstEntityBase * m_parent;
	std::string m_entity_type;
	ZstURI m_uri;
    ZstURI m_current_owner;
    std::mutex m_entity_lock;
};
