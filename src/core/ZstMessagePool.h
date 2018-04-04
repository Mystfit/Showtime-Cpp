#pragma once

#include <list>
#include "ZstMessage.h"

#define USE_MESSAGE_POOL false

class ZstMessage;

class ZstMessagePool {
public:
	ZST_EXPORT ZstMessagePool();
	ZST_EXPORT ~ZstMessagePool();

	ZST_EXPORT virtual void populate(int size) = 0;

	ZST_EXPORT virtual ZstMessage * get() = 0;
	
	//Release and reset a message
	ZST_EXPORT void release(ZstMessage * message);

protected:
	bool m_use_pool;
	std::list<ZstMessage*> m_message_pool;
};
