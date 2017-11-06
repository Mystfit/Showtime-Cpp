#pragma once

#include "ZstExports.h"
#include "ZstURI.h"
#include <unordered_map>

class ZstEntityBase {
public:
	friend class ZstEndpoint;
	
	//Base entity
    ZstEntityBase();
	ZST_EXPORT ZstEntityBase(const char * entity_type, const char * entity_name);
	ZST_EXPORT ZstEntityBase(const char * entity_type, const char * entity_name, ZstEntityBase * parent);
	ZST_EXPORT virtual ~ZstEntityBase();
    
    //Overridable init - must be called by overriden classes
	ZST_EXPORT virtual void init() = 0;
    
    //Entity type
	ZST_EXPORT const char * entity_type() const;
    
    //URI for this entity
	ZST_EXPORT const ZstURI & URI();
    
    //Entity flags
	ZST_EXPORT bool is_destroyed();
    
	//Override allocators so entity is created on DLL heap (Windows only - probably not compatible with SWIG)
	ZST_EXPORT void * operator new(size_t num_bytes);
	ZST_EXPORT void operator delete(void * p);
    
    //The parent of this entity
	ZST_EXPORT ZstEntityBase * parent() const;
    
    //Find a child in this entity by a URI
    ZST_EXPORT virtual ZstEntityBase * find_child_by_URI(const ZstURI & path) const;
    
    //Get a child by index
	ZST_EXPORT virtual ZstEntityBase * get_child_entity_at(int index) const;
    
    //Number of children owned by this entity
	ZST_EXPORT virtual const size_t num_children() const;
    
protected:
    char * m_entity_type;
    ZstURI m_uri;

private:
    //Manipulate the hierarchy of this entity
    void add_child(ZstEntityBase * child);
    void remove_child(ZstEntityBase * child);
    
    //Set entity status
    void set_destroyed();
    
    //Member vars
    std::unordered_map<ZstURI, ZstEntityBase*> m_children;
	
	bool m_is_destroyed;
	ZstEntityBase * m_parent;
};

//Typedef for entity factory functions
typedef void (*ZstEntityFactory)(const char*, ZstEntityBase*);

