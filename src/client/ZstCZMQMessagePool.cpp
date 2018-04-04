#pragma once

#include "ZstCZMQMessagePool.h"
#include "ZstCZMQMessage.h"

void ZstCZMQMessagePool::populate(int size)
{
	while (m_message_pool.size() < size) {
		m_message_pool.push_back(new ZstCZMQMessage());
	}
}

ZstMessage * ZstCZMQMessagePool::get()
{
	ZstMessage * msg = NULL;
	if (m_use_pool) {
		if (m_message_pool.empty()) {
			populate(MESSAGE_POOL_BLOCK);
		}
		msg = m_message_pool.front();
		m_message_pool.pop_front();
	}
	else {
		msg = new ZstCZMQMessage();
	}

	return msg;
}