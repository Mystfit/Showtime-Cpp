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
		std::shared_ptr<T> msg = NULL;
		while (this->m_message_pool.try_dequeue(msg)) {
			//delete msg;
		}
	}

	virtual void populate(int size)
	{
		for(size_t i = 0; i < size; ++i){
			m_message_pool.enqueue(std::make_shared<T>());
		}
	}

	virtual std::shared_ptr<T> get_msg()
	{
		std::shared_ptr<T> msg = NULL;
		this->m_message_pool.try_dequeue(msg);
		if (!msg) {
			populate(MESSAGE_POOL_BLOCK);
			this->m_message_pool.try_dequeue(msg);
		}
		
		return msg;
	}
	
	//Release and reset a message
	void release(std::shared_ptr<T> message)
	{
		message->reset();
		m_message_pool.enqueue(message);
	}

protected:
	moodycamel::ConcurrentQueue< std::shared_ptr<T> > m_message_pool;
};
