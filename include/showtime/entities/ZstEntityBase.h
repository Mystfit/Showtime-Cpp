#pragma once

#include <unordered_map>
#include <mutex>

#include "../ZstExports.h"
#include "../ZstURI.h"
#include "../ZstSerialisable.h"
#include "../ZstSynchronisable.h"
#include "../ZstBundle.hpp"
#include "../ZstCable.h"
#include "../ZstBundle.hpp"
#include "../adaptors/ZstEntityAdaptor.hpp"

//Forwards
template<typename T>
class ZstEventDispatcher;
class ZstEntityBase;
class ZstEntityFactory;

//Typedefs
typedef std::unordered_map<ZstURI, ZstEntityBase*, ZstURIHash> ZstEntityMap;

//Common bundle types
typedef ZstBundle<ZstURI> ZstURIBundle;
typedef ZstBundle<ZstEntityBase*> ZstEntityBundle;
typedef ZstBundle<ZstEntityFactory*> ZstEntityFactoryBundle;
typedef ZstBundle<ZstCable> ZstCableBundle;
typedef ZstBundleIterator<ZstEntityBase*> ZstEntityBundleIterator;
typedef ZstBundleIterator<ZstCable*> ZstCableBundleIterator;


class ZstEntityBase : public ZstSynchronisable, public ZstSerialisable {
public:
	//Include base class adaptors
	using ZstSynchronisable::add_adaptor;
	using ZstSynchronisable::remove_adaptor;

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
	ZST_EXPORT virtual ZstCableBundle & get_child_cables(ZstCableBundle & bundle) const;
	ZST_EXPORT virtual ZstEntityBundle&  get_child_entities(ZstEntityBundle & bundle, bool include_parent = true);
	    
	//Serialisation
	ZST_EXPORT void write_json(json & buffer) const override;
	ZST_EXPORT void read_json(const json & buffer) override;

	//Adaptors
    ZST_EXPORT void add_adaptor(ZstEntityAdaptor * adaptor);
    ZST_EXPORT void remove_adaptor(ZstEntityAdaptor * adaptor);
	ZST_EXPORT ZstEventDispatcher<ZstEntityAdaptor*> * entity_events();
	
protected:
	//Set entity status
	ZST_EXPORT void set_entity_type(const char * entity_type);
	ZST_EXPORT virtual void set_parent(ZstEntityBase* entity);
	ZST_EXPORT virtual void update_URI();
	ZST_EXPORT virtual void dispatch_destroyed() override;
    
    //Entity mutex
    mutable std::mutex m_entity_mtx;

private:
    ZstEventDispatcher<ZstEntityAdaptor*> * m_entity_events;
	ZstEntityBase * m_parent;
	std::string m_entity_type;
	ZstURI m_uri;
};
