#pragma once
#include <map>
#include <string>
#include <vector>
#include "ZstURI.h"
#include "ZstExports.h"
#include "entities/ZstEntityBase.h"

#define PERFORMER_ENTITY "performer"

class ZstPlug;
class ZstPerformer : public ZstEntityBase{
	public:
		ZstPerformer(const char * name);
		~ZstPerformer();
};
