#include <string>
#include <iostream>
#include <exception>
#include "ZstMessagePool.h"

using namespace std;

ZstMessagePool::ZstMessagePool() : m_use_pool(USE_MESSAGE_POOL)
{
}

ZstMessagePool::~ZstMessagePool()
{
	for (auto m : m_message_pool) {
		delete m;
	}
	m_message_pool.clear();
}

void ZstMessagePool::release(ZstMessage * message)
{
	if (m_use_pool) {
		message->reset();
		m_message_pool.push_back(message);
	}
	else {
		delete message;
	}
}
