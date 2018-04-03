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

void ZstMessagePool::populate(int size)
{
	while (m_message_pool.size() < size) {
		m_message_pool.push_back(new ZstMessage());
	}
}

ZstMessage * ZstMessagePool::get()
{
	ZstMessage * msg = NULL;
	if (m_use_pool) {
		if (m_message_pool.empty()) {
			msg = new ZstMessage();
		}
		else {
			msg = m_message_pool.front();
			m_message_pool.pop_front();
		}
	} else {
		msg = new ZstMessage();
	}

	return msg;
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
