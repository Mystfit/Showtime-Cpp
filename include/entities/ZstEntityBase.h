#pragma once

#include "ZstExports.h"
#include "Showtime.h"
#include "ZstURI.h"
#include "ZstPlug.h"
#include <vector>

class ZstEntityBase {
public:
	friend class ZstEndpoint;
	
	//Base entity
	ZST_EXPORT ZstEntityBase();
	ZST_EXPORT ZstEntityBase(const char * entity_type, const char * entity_name);
	ZST_EXPORT ZstEntityBase(const char * entity_type, const char * entity_name, ZstEntityBase * parent);
	ZST_EXPORT ~ZstEntityBase();
	ZST_EXPORT virtual void init();
	ZST_EXPORT const char * entity_type() const;
	ZST_EXPORT ZstURI URI();
	ZST_EXPORT bool is_registered();
	ZST_EXPORT ZstEntityBase * parent() const;

private:
	Str255 m_entity_type;
	ZstURI m_uri;
	bool m_is_registered;
	ZstEntityBase * m_parent;
};