#pragma once

#include <czmq.h>
#include "../core/ZstMessage.h"

class ZstCZMQMessage : public ZstMessage {
	ZstCZMQMessage();
	~ZstCZMQMessage();
};