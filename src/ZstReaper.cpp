#include "ZstReaper.h"
#include "entities/ZstEntityBase.h"

ZstReaper::ZstReaper()
{
}


ZstReaper::~ZstReaper()
{
}

void ZstReaper::join()
{
	while (m_destroying_entities.size() > 0) {

	}
}


void ZstReaper::destroy()
{
	join();
	ZstActor::destroy();
}
