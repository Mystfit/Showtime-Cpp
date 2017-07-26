#pragma once

#include "entities\ZstEntityBase.h"

class ZstPatch : public virtual ZstEntityBase {
public:
	ZST_EXPORT virtual const ZstEntityBase * get_child_entity_at(int index) const;
	ZST_EXPORT virtual const size_t num_children() const;

protected:
	std::vector<ZstEntityBase*> m_children;
};