#include "entities\ZstPatch.h"

const ZstEntityBase * ZstPatch::get_child_entity_at(int index) const
{
	return m_children[index];
}

const size_t ZstPatch::num_children() const
{
	return m_children.size();
}
