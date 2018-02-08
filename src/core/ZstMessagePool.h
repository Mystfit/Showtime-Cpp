#pragma once

#include <list>
#include <map>
#include <cf/cf/cfuture.h>
#include <cf/time_watcher.h>
#include "ZstMessage.h"

#define USE_MESSAGE_POOL false

typedef cf::promise<ZstMsgKind> MessagePromise;
typedef cf::future<ZstMsgKind> MessageFuture;

struct ZstTimeoutException : std::runtime_error { 
	using std::runtime_error::runtime_error;
};

class ZstMessage;

class ZstMessagePool {
public:
	ZST_EXPORT ZstMessagePool();
	ZST_EXPORT ~ZstMessagePool();

	ZST_EXPORT void populate(int size);

	//Obtain a blank message
	ZST_EXPORT ZstMessage * get();
	
	//Register a message that will receive a future response
	ZST_EXPORT MessageFuture register_future(ZstMessage * msg, bool timeout = false);

	//Check for existing message promises matching a message ID
	ZST_EXPORT int process_message_promise(ZstMessage * msg);

	//Release and reset a message
	ZST_EXPORT void release(ZstMessage * message);

private:
	bool m_use_pool;
	std::list<ZstMessage*> m_message_pool;
	std::unordered_map<std::string, MessagePromise > m_promise_messages;
	cf::time_watcher m_timeout_watcher;
};
