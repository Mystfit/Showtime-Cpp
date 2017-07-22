#include "ZstPerformer.h"
#include "ZstPlug.h"

using namespace std;

ZstPerformer::ZstPerformer(const char * name) : 
	ZstEntityBase(ZstEntityBehaviour::PERFORMER, PERFORMER_ENTITY, name) {
}

ZstPerformer::~ZstPerformer()
{
}

