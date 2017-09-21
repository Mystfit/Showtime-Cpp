#pragma once

#include "ZstExports.h"
#include "ZstURI.h"
#include <unordered_map>

class ZstEntityBase {
public:
	friend class ZstEndpoint;
	
	//Base entity
	ZST_EXPORT ZstEntityBase(const char * entity_type, const char * entity_name);
	ZST_EXPORT ZstEntityBase(const char * entity_type, const char * entity_name, ZstEntityBase * parent);
	ZST_EXPORT virtual ~ZstEntityBase();
	ZST_EXPORT virtual void init() =0;
    ZST_EXPORT virtual void activate();
	ZST_EXPORT const char * entity_type() const;
	ZST_EXPORT const ZstURI & URI();
	ZST_EXPORT bool is_registered();
	ZST_EXPORT bool is_destroyed();
	ZST_EXPORT void set_destroyed();
    ZST_EXPORT bool is_proxy();

	//Override allocators so entity is created on DLL heap (Windows only)
	ZST_EXPORT void * operator new(size_t num_bytes);
	ZST_EXPORT void operator delete(void * p);

	ZST_EXPORT ZstEntityBase * parent() const;
    ZST_EXPORT virtual ZstEntityBase * find_child_by_URI(const ZstURI & path) const;
	ZST_EXPORT virtual ZstEntityBase * get_child_entity_at(int index) const;
	ZST_EXPORT virtual const size_t num_children() const;
    
protected:
    bool m_is_proxy;
	bool m_is_registered;

private:
    void add_child(ZstEntityBase * child);
    void remove_child(ZstEntityBase * child);
    std::unordered_map<ZstURI, ZstEntityBase*> m_children;
	char * m_entity_type;
	ZstURI m_uri;
	bool m_is_destroyed;
	ZstEntityBase * m_parent;
};
