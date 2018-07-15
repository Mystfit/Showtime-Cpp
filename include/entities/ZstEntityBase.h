#pragma once

#include <unordered_map>
#include <ZstExports.h>
#include <ZstURI.h>
#include <ZstSerialisable.h>
#include <ZstSynchronisable.h>
#include <adaptors/ZstEntityAdaptor.hpp>

//Forwards
class ZstCableBundle;

template<typename T>
class ZstEventDispatcher;

class ZstEntityBase : public ZstSynchronisable, public ZstSerialisable {
public:
	friend class ZstClient;
	friend class ZstContainer;

	//Base entity
	ZST_EXPORT ZstEntityBase(const char * entity_name);
	ZST_EXPORT ZstEntityBase(const ZstEntityBase & other);
	ZST_EXPORT virtual ~ZstEntityBase();
	
	//TODO: This is handled by whatever DLL or SO owns the concrete implemetation of this entity
	ZST_EXPORT void * operator new(size_t num_bytes);
	ZST_EXPORT void operator delete(void * p);
    	
	//The parent of this entity
	ZST_EXPORT ZstEntityBase * parent() const;
    
    //Entity type
	ZST_EXPORT const char * entity_type() const;
    
    //URI for this entity
	ZST_EXPORT const ZstURI & URI() const;

	//Cable management
	ZST_EXPORT virtual ZstCableBundle * acquire_cable_bundle();
	ZST_EXPORT void release_cable_bundle(ZstCableBundle * cables);
	ZST_EXPORT virtual void disconnect_cables() {};
	    
	//Serialisation
	ZST_EXPORT virtual void write(std::stringstream & buffer) const override;
	ZST_EXPORT virtual void read(const char * buffer, size_t length, size_t & offset) override;

	//Adaptors
    ZST_EXPORT static void add_adaptor(ZstEntityBase * self, ZstEntityAdaptor * adaptor);
    ZST_EXPORT static void remove_adaptor(ZstEntityBase * self, ZstEntityAdaptor * adaptor);

	ZST_EXPORT virtual void add_adaptor_to_children(ZstSynchronisableAdaptor * adaptor);
	ZST_EXPORT virtual void remove_adaptor_from_children(ZstSynchronisableAdaptor * adaptor);
	
protected:
	//Set entity status
	ZST_EXPORT void set_entity_type(const char * entity_type);
	ZST_EXPORT virtual void set_parent(ZstEntityBase* entity);
	ZST_EXPORT virtual ZstCableBundle * get_child_cables(ZstCableBundle * bundle);
    ZST_EXPORT ZstEventDispatcher<ZstEntityAdaptor*> * entity_events();
    
private:
    ZST_EXPORT void update_URI();
    ZstEventDispatcher<ZstEntityAdaptor*> * m_entity_events;
	ZstEntityBase * m_parent;
	char * m_entity_type;
	ZstURI m_uri;
};

//Typedefs
typedef std::unordered_map<ZstURI, ZstEntityBase*, ZstURIHash> ZstEntityMap;
