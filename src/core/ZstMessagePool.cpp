#include <string>
#include <iostream>
#include <exception>
#include "ZstMessage.h"
#include "ZstMessagePool.h"


using namespace std;

ZstMessagePool::ZstMessagePool()
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
	//ZstMessage * msg = NULL;
	//if (m_message_pool.empty()) {
	//	msg = new ZstMessage();
	//}
	//else {
	//	msg = m_message_pool.front();
	//	m_message_pool.pop_front();
	//}

	return new ZstMessage();
}

MessageFuture ZstMessagePool::register_future(ZstMessage * msg)
{
	string id = string(msg->id());
	m_promise_messages[id] = MessagePromise();
	return m_promise_messages[id].get_future();
}

int ZstMessagePool::process_message_promise(ZstMessage * msg)
{
	int status = 0;
	try {
		std::string id = std::string(msg->id());
		m_promise_messages.at(id).set_value(msg->kind());

		//Clear completed promise when finished
		m_promise_messages.erase(msg->id());
		status = 1;
	}
	catch (out_of_range e) {
		status = -1;
	}

	return status;
}

void ZstMessagePool::release(ZstMessage * message)
{
	delete message;
	//message->reset();
	//m_message_pool.push_back(message);
}
