#pragma once

#include "entities/ZstComponent.h"

#define FILTER_TYPE "filter"

class FilterComputeCallback;
class ZstComponent;

class ZstFilter : public ZstComponent {
public:
	ZST_EXPORT ZstFilter(const char * entity_type, const char * name);
	ZST_EXPORT ZstFilter(const char * entity_type, const char * name, ZstEntityBase * parent);
};
