#pragma once

#include <list>
#include "ZstMessage.h"

#define USE_MESSAGE_POOL false

class ZstMessage;

class ZstMessagePool {
public:
	ZST_EXPORT ZstMessagePool();
	ZST_EXPORT ~ZstMessagePool();

	ZST_EXPORT void populate(int size);

	//Obtain a blank message
	ZST_EXPORT ZstMessage * get();
	
	//Release and reset a message
	ZST_EXPORT void release(ZstMessage * message);

private:
	bool m_use_pool;
	std::list<ZstMessage*> m_message_pool;
};
