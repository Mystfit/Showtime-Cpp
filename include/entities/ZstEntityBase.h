#pragma once

#include <unordered_map>
#include <ZstExports.h>
#include <ZstURI.h>
#include <ZstSerialisable.h>
#include <ZstSynchronisable.h>
#include <ZstBundle.hpp>
#include <ZstCable.h>
#include <adaptors/ZstEntityAdaptor.hpp>

//Forwards
template<typename T>
class ZstEventDispatcher;
class ZstEntityBase;

//Typedefs
typedef ZstBundle<ZstCable*> ZstCableBundle;
typedef ZstBundle<ZstEntityBase*> ZstEntityBundle;
typedef std::unordered_map<ZstURI, ZstEntityBase*, ZstURIHash> ZstEntityMap;


class ZstEntityBase : public ZstSynchronisable, public ZstSerialisable {
public:
	friend class ZstClient;
	friend class ZstContainer;

	//Include base class adaptors
	using ZstSynchronisable::add_adaptor;
	using ZstSynchronisable::remove_adaptor;

	//Base entity
	ZST_EXPORT ZstEntityBase(const char * entity_name);
	ZST_EXPORT ZstEntityBase(const ZstEntityBase & other);
	ZST_EXPORT virtual ~ZstEntityBase();
	
	//TODO: This is handled by whatever DLL or SO owns the concrete implemetation of this entity
	//ZST_EXPORT void * operator new(size_t num_bytes);
	//ZST_EXPORT void operator delete(void * p);
    	
	//The parent of this entity
	ZST_EXPORT ZstEntityBase * parent() const;
    
    //Entity type
	ZST_EXPORT const char * entity_type() const;
    
    //URI for this entity
	ZST_EXPORT const ZstURI & URI() const;

	//Iterate
	ZST_EXPORT ZstEntityBundle * aquire_child_bundle();
	ZST_EXPORT static void release_child_bundle(ZstEntityBundle * bundle);

	//Cable management
	ZST_EXPORT virtual ZstCableBundle * aquire_cable_bundle();
	ZST_EXPORT static void release_cable_bundle(ZstCableBundle * bundle);
	    
	//Serialisation
	ZST_EXPORT virtual void write(std::stringstream & buffer) const override;
	ZST_EXPORT virtual void read(const char * buffer, size_t length, size_t & offset) override;

	//Adaptors
	//ZST_EXPORT virtual void add_adaptor(ZstSynchronisableAdaptor * adaptor, bool recursive = false) override;
	//ZST_EXPORT virtual void remove_adaptor(ZstSynchronisableAdaptor * adaptor, bool recursive = false) override;
    ZST_EXPORT virtual void add_adaptor(ZstEntityAdaptor * adaptor);
    ZST_EXPORT virtual void remove_adaptor(ZstEntityAdaptor * adaptor);
	
protected:
	//Set entity status
	ZST_EXPORT void set_entity_type(const char * entity_type);
	ZST_EXPORT virtual void set_parent(ZstEntityBase* entity);
	ZST_EXPORT virtual ZstCableBundle * get_child_cables(ZstCableBundle * bundle);
	ZST_EXPORT virtual ZstEntityBundle * get_child_entities(ZstEntityBundle * bundle);
    ZST_EXPORT ZstEventDispatcher<ZstEntityAdaptor*> * entity_events();
    
private:
    ZST_EXPORT void update_URI();
    ZstEventDispatcher<ZstEntityAdaptor*> * m_entity_events;
	ZstEntityBase * m_parent;
	char * m_entity_type;
	ZstURI m_uri;
	ZstEntityBase * m_current_child_head;
};



// Bundle unique wrappers

class ZstEntityBundleScoped
{
public:
	ZstEntityBundleScoped() = delete;
	ZST_EXPORT ZstEntityBundleScoped(ZstEntityBase * entity, bool include_parent = true);
	ZST_EXPORT ZstBundleIterator<ZstEntityBase*> begin() {
		return m_bundle->begin();
	}
	ZST_EXPORT ZstBundleIterator<ZstEntityBase*> end() {
		return m_bundle->end();
	}

private:
	std::unique_ptr< ZstEntityBundle, void(*)(ZstEntityBundle*)> m_bundle;
};


class ZstCableBundleScoped
{
public:
	ZstCableBundleScoped() = delete;
	ZST_EXPORT ZstCableBundleScoped(ZstEntityBase * entity);
	ZST_EXPORT ZstBundleIterator<ZstCable*> begin() {
		return m_bundle->begin();
	}
	ZST_EXPORT ZstBundleIterator<ZstCable*> end() {
		return m_bundle->end();
	}
private:
	std::unique_ptr< ZstCableBundle, void(*)(ZstCableBundle*)> m_bundle;
};
