#pragma once

#include <ZstConstants.h>
#include <list>
#include <mutex>

template<typename T>
class ZstMessagePool 
{
public:
	ZstMessagePool()
	{
		populate(MESSAGE_POOL_BLOCK);
	}

	~ZstMessagePool() {
		std::unique_lock<std::mutex> lock(m_mutex);
		for (auto m : m_message_pool) {
			delete m;
		}
		m_message_pool.clear();
	}

	virtual void populate(int size)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
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

		//Lock and pop message from list
		std::unique_lock<std::mutex> lock(m_mutex);
		msg = m_message_pool.front();
		m_message_pool.pop_front();
		lock.unlock();
		
		return msg;
	}
	
	//Release and reset a message
	void release(T* message)
	{
		//Flag message as being inactive so that it can be cleaned up in the future
		message->set_inactive();
		message->reset();
			
		std::unique_lock<std::mutex> lock(m_mutex);
		m_message_pool.push_back(message);
	}

protected:
	std::list<T*> m_message_pool;
	std::mutex m_mutex;
};
