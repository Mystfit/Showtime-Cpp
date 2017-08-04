#include "entities/ZstFilter.h"

ZstFilter::ZstFilter(const char * entity_type, const char * name) : ZstComponent(entity_type, name)
{
	init();
}

ZstFilter::ZstFilter(const char * entity_type, const char * name, ZstEntityBase * parent) : ZstComponent(entity_type, name, parent)
{
	init();
}