#pragma once

#include <unordered_map>
#include <ZstExports.h>
#include <ZstURI.h>
#include <ZstSerialisable.h>
#include <ZstSynchronisable.h>

//Forwards
class ZstCableBundle;

//Typedefs
typedef std::unordered_map<ZstURI, ZstEntityBase*, ZstURIHash> ZstEntityMap;

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
	ZST_EXPORT const ZstURI & URI();
    
    //Entity flags
	ZST_EXPORT bool is_destroyed();

	//Cable management
	ZST_EXPORT virtual ZstCableBundle * acquire_cable_bundle();
	ZST_EXPORT void release_cable_bundle(ZstCableBundle * cables);
	ZST_EXPORT virtual void disconnect_cables() {};
	    
	//Serialisation
	ZST_EXPORT virtual void write(std::stringstream & buffer) override;
	ZST_EXPORT virtual void read(const char * buffer, size_t length, size_t & offset) override;
	
protected:
	//Set entity status
	ZST_EXPORT void set_entity_type(const char * entity_type);
	ZST_EXPORT virtual void set_parent(ZstEntityBase* entity);
	ZST_EXPORT void set_destroyed();
	ZST_EXPORT virtual ZstCableBundle * get_child_cables(ZstCableBundle * bundle);

private:
    ZST_EXPORT void update_URI();
    
	bool m_is_destroyed;
	ZstEntityBase * m_parent;
	char * m_entity_type;
	ZstURI m_uri;
};
