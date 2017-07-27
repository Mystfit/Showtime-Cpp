#pragma once

#include "entities\ZstEntityBase.h"

#define PATCH_TYPE "patch"

class ZstPatch : public virtual ZstEntityBase {
public:
	ZST_EXPORT ZstPatch(const char * name) : ZstEntityBase(PATCH_TYPE, name) {};
	ZST_EXPORT ZstPatch(const char * name, ZstEntityBase * parent) : ZstEntityBase(PATCH_TYPE, name, parent) {};
	
	ZST_EXPORT virtual const ZstEntityBase * get_child_entity_at(int index) const;
	ZST_EXPORT virtual const size_t num_children() const;

protected:
	std::vector<ZstEntityBase*> m_children;
};