#pragma once

#include <ZstConstants.h>
#include <list>

template<typename T>
class ZstMessagePool 
{
public:
	ZstMessagePool()
	{
		populate(MESSAGE_POOL_BLOCK);
	}

	~ZstMessagePool() {
		for (auto m : m_message_pool) {
			delete m;
		}
		m_message_pool.clear();
	}

	virtual void populate(int size)
	{
		while (m_message_pool.size() < size) {
			m_message_pool.push_back(new T());
		}
	}

	virtual T* get_msg()
	{
		T* msg = NULL;
		if (m_message_pool.empty()) {
			populate(MESSAGE_POOL_BLOCK);
		}
		msg = m_message_pool.front();
		m_message_pool.pop_front();
		msg->reset();

		return msg;
	}
	
	//Release and reset a message
	void release(T* message)
	{
		message->set_inactive();
		m_message_pool.push_back(message);
	}

protected:
	std::list<T*> m_message_pool;
};
