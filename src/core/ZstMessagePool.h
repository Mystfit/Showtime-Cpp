#pragma once

#include <list>
#include <map>
#include <cf/cfuture.h>
#include "ZstMessage.h"

typedef cf::promise<ZstMessage::Kind> MessagePromise;
typedef cf::future<ZstMessage::Kind> MessageFuture;

class ZstMessage;

class ZstMessagePool {
public:
	ZST_EXPORT ZstMessagePool();
	ZST_EXPORT ~ZstMessagePool();

	ZST_EXPORT void populate(int size);

	//Obtain a blank message
	ZST_EXPORT ZstMessage * get();
	
	//Register a message that will receive a future response
	ZST_EXPORT MessageFuture register_future(ZstMessage * msg);

	//Check for existing message promises matching a message ID
	ZST_EXPORT int process_message_promise(ZstMessage * msg);

	//Release and reset a message
	ZST_EXPORT void release(ZstMessage * message);

private:
	std::list<ZstMessage*> m_message_pool;
	std::unordered_map<std::string, MessagePromise > m_promise_messages;
};