#pragma once

#include <list>
#include <ZstExports.h>

#define USE_MESSAGE_POOL false

template<typename T>
class ZstMessagePool {
public:
	ZST_EXPORT ZstMessagePool() : m_use_pool(USE_MESSAGE_POOL)
	{
		populate(MESSAGE_POOL_BLOCK);
	}

	ZST_EXPORT ~ZstMessagePool() {
		for (auto m : m_message_pool) {
			delete m;
		}
		m_message_pool.clear();
	}

	ZST_EXPORT virtual void populate(int size)
	{
		while (m_message_pool.size() < size) {
			m_message_pool.push_back(new T());
		}
	}

	ZST_EXPORT virtual T* get_msg()
	{
		T* msg = NULL;
		if (m_use_pool) {
			if (m_message_pool.empty()) {
				populate(MESSAGE_POOL_BLOCK);
			}
			msg = m_message_pool.front();
			m_message_pool.pop_front();
			msg->reset();
		}
		else {
			msg = new T();
		}

		return msg;
	}
	
	//Release and reset a message
	ZST_EXPORT void release(T* message)
	{
		if (m_use_pool) {
			m_message_pool.push_back(message);
		}
		else {
			delete message;
		}
	}

protected:
	bool m_use_pool;
	std::list<T*> m_message_pool;
};
