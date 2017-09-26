#pragma once

#include "ZstActor.h"
#include "Queue.h"

class ZstEntityBase;

class ZstReaper : public ZstActor {
public:
	ZstReaper();
	~ZstReaper();

	void join();

	void destroy() override;
private:
	Queue<ZstEntityBase*> m_destroying_entities;
};