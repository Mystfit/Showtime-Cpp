#include "ZstPatch.h"

ZstEntityBase * ZstPatch::get_child_entity_at(int index)
{
	return m_children[index];
}

size_t ZstPatch::num_children()
{
	return m_children.size();
}
