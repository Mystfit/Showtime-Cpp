#pragma once

#include "entities\ZstPlug.h"

class ZstGraphSender {
public:
	virtual void send_to_graph(ZstPlug * plug) = 0;
};