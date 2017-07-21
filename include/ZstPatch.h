#pragma once

#include <vector>
#include "ZstEntityBase.h"

class ZstPatch : public ZstEntityBase {
public:
	ZstEntityBase * get_child_entity_at(int index);
	size_t num_children();
private:
	std::vector<ZstEntityBase*> m_children;
};