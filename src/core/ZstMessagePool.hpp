#pragma once

#include <ZstConstants.h>
#include <list>
#include <concurrentqueue.h>

template<typename T>
class ZstMessagePool 
{
public:
	ZstMessagePool()
	{
		populate(MESSAGE_POOL_BLOCK);
	}

	~ZstMessagePool() {
		T* msg = NULL;
		while (this->m_message_pool.try_dequeue(msg)) {
			delete msg;
		}
	}

	virtual void populate(int size)
	{
		for(size_t i = 0; i < size; ++i){
			m_message_pool.enqueue(new T());
		}
	}

	virtual T* get_msg()
	{
		T* msg = NULL;
		this->m_message_pool.try_dequeue(msg);
		if (!msg) {
			populate(MESSAGE_POOL_BLOCK);
			this->m_message_pool.try_dequeue(msg);
		}
		
		return msg;
	}
	
	//Release and reset a message
	void release(T* message)
	{
		//Flag message as being inactive so that it can be cleaned up in the future
		message->set_inactive();
		message->reset();
			
		m_message_pool.enqueue(message);
	}

protected:
	moodycamel::ConcurrentQueue<T*> m_message_pool;
};
